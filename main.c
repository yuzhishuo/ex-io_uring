#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* If your compilation fails because the header file below is missing,
 * your kernel is probably too old to support io_uring.
 * */
#include <linux/io_uring.h>

#define QUEUE_DEPTH 1
#define BLOCK_SZ    1024
const char * file_path = "./log.txt";
int fd = -1;

/* This is x86 specific */
#define read_barrier()  __asm__ __volatile__("":::"memory")
#define write_barrier() __asm__ __volatile__("":::"memory")

struct app_io_sq_ring {
    // 环头部偏移量，因为在换的开始还要存别的东西
    unsigned *head;
    // 环尾部偏移量
    unsigned *tail;


    unsigned *ring_mask;
    // 环的大小
    unsigned *ring_entries;
    
    /*
        * Number of invalid entries dropped by the kernel due to
        * invalid index stored in array
        *
        * Written by the kernel, shouldn't be modified by the
        * application (i.e. get number of "new events" by comparing to
        * cached value).
        *
        * After a new SQ head value was read by the application this
        * counter includes all submissions that were dropped reaching
        * the new SQ head (and possibly more).
    */  // 这个没有， 在别的文章里面有
    // u32                     dropped;

    /*
    * Runtime flags
    *
    * Written by the kernel, shouldn't be modified by the
    * application.
    *
    * The application needs a full memory barrier before checking
    * for IORING_SQ_NEED_WAKEUP after updating the sq tail.
    */
    unsigned *flags;

    // sq_ring 拆分之后，使用的是 array 的零长度数组来指向 sqes 的另外一块内存区域，数据结构的描述都很详细，没有需要特别说明的地方。
    unsigned *array;
};

struct app_io_cq_ring {
    unsigned *head;
    unsigned *tail;
    unsigned *ring_mask;
    unsigned *ring_entries;
    /*
    * Number of completion events lost because the queue was full;
    * this should be avoided by the application by making sure
    * there are not more requests pending thatn there is space in
    * the completion queue.
    *
    * Written by the kernel, shouldn't be modified by the
    * application (i.e. get number of "new events" by comparing to
    * cached value).
    *
    * As completion events come in out of order this counter is not
    * ordered with any other data.
    */ // 这个也没有
    // u32                     overflow;
    // cq_ring 自我包含了 cqes，而不是拆分开来，各自的段也都描述得很清晰，没有特别需要解释的。cq_ring 自我包含了 cqes，而不是拆分开来，各自的段也都描述得很清晰，没有特别需要解释的。
    struct io_uring_cqe *cqes;
};

struct submitter {
    int ring_fd;
    struct app_io_sq_ring sq_ring;
    struct io_uring_sqe *sqes;
    struct app_io_cq_ring cq_ring;
};

struct file_info {
    off_t file_sz;
    struct iovec iovecs[];      /* Referred by readv/writev */
};

/*
 * This code is written in the days when io_uring-related system calls are not
 * part of standard C libraries. So, we roll our own system call wrapper
 * functions.
 * */

/**
 * @brief 在内存中创建一个区域，该区域分为 SQ (提交队列)， CQ(完成队列)，SQEs(一系列的提交队列项)
 *         其中，SQEs该数组的目的是方便通过环形缓冲区提交内存上不连续的请求，即内核的响应请求的顺序是不确定的，导致在 SEQs 中插入新请求的位置可能是离散的。
 *         SQ 和 CQ 中每个节点保存的都是 SQEs 数组的索引，而不是实际的请求，实际的请求只保存在 SQEs 数组中。这样在提交请求时，就可以批量提交一组 SQEs 上不连续的请求。
 * 
 * @param entries 建立一个带有至少entries个实体的提交队列和完成队列 
 * @param p     p参数可以让应用程序来配置io_uring实例。这个参数也用于内核返回对提交队列和完成队列的配置信息。
 * @return int  对应区域的 fd（成为 ringfd），应用程序通过该 fd 进行 mmap，实现和 kernel 的内存共享。
 */

// struct io_uring_params {
// 	__u32 sq_entries;
// 	__u32 cq_entries;
// 	__u32 flags;           
// 	__u32 sq_thread_cpu;  // 可选参数，在轮询模式下，内核会启动一个线程，该线程会在sq_thread_cpu指定的CPU上运行。来轮询sq_ring中的请求。
// 	__u32 sq_thread_idle; // 可选参数，指定提交队列的空闲时间, 在超过这个时间没有Poll到任何请求，则该 SQ 线程将被挂起
// 	__u32 features;
// 	__u32 resv[4];
// 	struct io_sqring_offsets sq_off;
// 	struct io_cqring_offsets cq_off;
// };

int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
    return (int) syscall(__NR_io_uring_setup, entries, p);
}


/**
 * @brief 用于使用提交队列和完成队列发起和完成I/O，这两个队列是由io_uring_setup()所建立的。一次调用就可以同时完成新I/O提交和等待I/O完成，包括当前和之前io_uring_enter()调用提交的I/O。
 * 
 * @param ring_fd  是由io_uring_setup()调用返回的文件描述符。
 * @param to_submit  to_submit 指定了要从提交队列里提交的I/O的数量。如果应用指定了min_complete, 那么这个调用会在返回前等待至少min_complete个事件。 
 *                   如果io_uring实例配置成了轮询模式，那么min_complete参数的含义会稍有不同。传入0表示要内核返回任何已经完成了的事件， 而不要阻塞。
 *                   如果min_complete非0，而且存在已经完成了的事件，内核仍然会立即返回。如果没有就绪的完成事件，这个函数将会进行轮询直到有一个或多个完成事件就绪，或者直到进程超过了它被调度的时间片。·
 * @param min_complete 对于中断驱动的I/O，应用程序也可以检查完成队列来获得完成事件而无需进入内核。
 * @param flags 
 * @return int 
 */

// 当SQ 线程睡眠时候，通过调用  io_uring_enter() 并且设置 IORING_ENTER_SQ_WAKEUP 来唤醒SQ线程。用户态可以通过 flag 变量 捕获到线程的状态
/*
*  在提交 IO 的时候，如果出现了没有空闲的 SEQ entry 来提交新的请求的时候，应用程序不知道什么时候有空闲的情况，只能不断重试。
   为解决这种场景的问题，可以在调用 io_uring_enter 的时候设置 IORING_ENTER_SQ_WAIT 标志位，当提交新请求的时候，它会等到至少有一个新的 SQ entry 能使用的时候才返回。
*/ 
int io_uring_enter(int ring_fd, unsigned int to_submit,
                          unsigned int min_complete, unsigned int flags)
{
    return (int) syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                   flags, NULL, 0);
}

/*
 * Returns the size of the file whose open file descriptor is passed in.
 * Properly handles regular file and block devices as well. Pretty.
 * */

off_t get_file_size(int fd) {
    struct stat st;

    if(fstat(fd, &st) < 0) {
        perror("fstat");
        return -1;
    }
    if (S_ISBLK(st.st_mode)) { // S_ISBLK
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
            perror("ioctl");
            return -1;
        }
        return bytes;
    } else if (S_ISREG(st.st_mode)) // 是否是一个普通文件
        return st.st_size;

    return -1;
}

/*
 * io_uring requires a lot of setup which looks pretty hairy, but isn't all
 * that difficult to understand. Because of all this boilerplate code,
 * io_uring's author has created liburing, which is relatively easy to use.
 * However, you should take your time and understand this code. It is always
 * good to know how it all works underneath. Apart from bragging rights,
 * it does offer you a certain strange geeky peace.
 * */

int app_setup_uring(struct submitter *s) {
    struct app_io_sq_ring *sring = &s->sq_ring;
    struct app_io_cq_ring *cring = &s->cq_ring;
    struct io_uring_params p;
    void *sq_ptr, *cq_ptr;

    /*
     * We need to pass in the io_uring_params structure to the io_uring_setup()
     * call zeroed out. We could set any flags if we need to, but for this
     * example, we don't.
     * */
    memset(&p, 0, sizeof(p));
    s->ring_fd = io_uring_setup(QUEUE_DEPTH, &p);

    char buf[4096];

    int j =0;
    j =sprintf(buf,  "io_uring_setup: %d\n", s->ring_fd);
    j+= sprintf(buf+j, "io_uring_params, sq , entries is : %d\n", p.sq_entries);
    j+= sprintf(buf+j, "io_uring_params, cq , entries is : %d\n", p.cq_entries); // cq 数目 是 sq 的两倍
    j+= sprintf(buf+j, "io_uring_params, sq , sq_thread_cpu is : %d\n", p.sq_thread_cpu);
    j+= sprintf(buf+j, "io_uring_params, sq , sq_thread_idle is : %d\n", p.sq_thread_idle);
    j+= sprintf(buf+j, "io_uring_params, flags : %d\n", p.flags);
    j+= sprintf(buf+j, "io_uring_params, feature: %d\n", p.features);
    j+= sprintf(buf+j, "------------------------------------------------------");
    write(fd, buf, j);
    if (s->ring_fd < 0) {
        perror("io_uring_setup");
        return 1;
    }

    /*
     * io_uring communication happens via 2 shared kernel-user space ring buffers,
     * which can be jointly mapped with a single mmap() call in recent kernels. 
     * While the completion queue is directly manipulated, the submission queue 
     * has an indirection array in between. We map that in as well.
     * */

    char buf1[4096];
    int j1 =0;
    j1 =sprintf(buf1,  "io_uring_setup: %d\n", s->ring_fd);
    j1+= sprintf(buf1+j1, "io_uring_params, p.sq_off.head is : %d\n", p.sq_off.head);
    j1+= sprintf(buf1+j1, "io_uring_params, p.sq_off.tail is : %d\n", p.sq_off.tail);
    j1+= sprintf(buf1+j1, "io_uring_params, p.sq_off.array is : %d\n", p.sq_off.array);
    j1+= sprintf(buf1+j1, "io_uring_params, p.cq_off.cqes is : %d\n", p.cq_off.cqes);
    j1+= sprintf(buf1+j1, "------------------------------------------------------\n");
    write(fd, buf1, j1);

    int sring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned); // SQ Array 是指向 SQ Entry 的指针数组， 从赋值看出 array mapp 映射的最后一个区域 
    int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe); // CQ 中只有 cq的数据结构所以大小依靠 cq_entries * io_uring_cqe 加上 cq_off.cqes的偏移量

    char buf2[4096];
    int j2 =0;
    j2 =sprintf(buf2,  "sring_sz: %d\n", sring_sz);
    j2+= sprintf(buf2+j2, "cring_sz: %d\n", cring_sz);
    j2+= sprintf(buf2+j2, "------------------------------------------------------\n");
    write(fd, buf2, j2);

    /* In kernel version 5.4 and above, it is possible to map the submission and 
     * completion buffers with a single mmap() call. Rather than check for kernel 
     * versions, the recommended way is to just check the features field of the 
     * io_uring_params structure, which is a bit mask. If the 
     * IORING_FEAT_SINGLE_MMAP is set, then we can do away with the second mmap()
     * call to map the completion ring.
     * */
    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        if (cring_sz > sring_sz) {
            sring_sz = cring_sz;
        }
        cring_sz = sring_sz;
    }

    /* Map in the submission and completion queue ring buffers.
     * Older kernels only map in the submission queue, though.
     * */
    sq_ptr = mmap(0, sring_sz, PROT_READ | PROT_WRITE, 
            MAP_SHARED /*与其它所有映射这个对象的进程共享映射空间。对共享区的写入，相当于输出到文件。
                            直到msync()或者munmap()被调用，文件实际上不会被更新。*/
             | MAP_POPULATE /*
                为文件映射通过预读的方式准备好页表。随后对映射区的访问不会被页违例阻塞。
                v. （大批人或动物）居住于，生活于；充满，出现于（某地方，领域）；迁移，殖民于；（给文件）增添数据，输入数据
             */ ,

            s->ring_fd, IORING_OFF_SQ_RING /*被映射文件内容的起点， 针对 提交队列*/);
    if (sq_ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        cq_ptr = sq_ptr;
    } else {
        /* Map in the completion queue ring buffer in older kernels separately */
        cq_ptr = mmap(0, cring_sz, PROT_READ | PROT_WRITE, 
                MAP_SHARED | MAP_POPULATE,
                s->ring_fd, IORING_OFF_CQ_RING);
        if (cq_ptr == MAP_FAILED) {
            perror("mmap");
            return 1;
        }
    }
    /* Save useful fields in a global app_io_sq_ring struct for later
     * easy reference */
    sring->head = sq_ptr + p.sq_off.head;
    sring->tail = sq_ptr + p.sq_off.tail;
    sring->ring_mask = sq_ptr + p.sq_off.ring_mask;
    sring->ring_entries = sq_ptr + p.sq_off.ring_entries;
    sring->flags = sq_ptr + p.sq_off.flags;
    sring->array = sq_ptr + p.sq_off.array;

    /* Map in the submission queue entries array */
    s->sqes = mmap(0, p.sq_entries * sizeof(struct io_uring_sqe),
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
            s->ring_fd, IORING_OFF_SQES);
    if (s->sqes == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Save useful fields in a global app_io_cq_ring struct for later
     * easy reference */
    cring->head = cq_ptr + p.cq_off.head;
    cring->tail = cq_ptr + p.cq_off.tail;
    cring->ring_mask = cq_ptr + p.cq_off.ring_mask;
    cring->ring_entries = cq_ptr + p.cq_off.ring_entries;
    cring->cqes = cq_ptr + p.cq_off.cqes;

    return 0;
}

/*
 * Output a string of characters of len length to stdout.
 * We use buffered output here to be efficient,
 * since we need to output character-by-character.
 * */
void output_to_console(char *buf, int len) {
    while (len--) {
        fputc(*buf++, stdout);
    }
}

/*
 * Read from completion queue.
 * In this function, we read completion events from the completion queue, get
 * the data buffer that will have the file data and print it to the console.
 * */

void read_from_cq(struct submitter *s) {
    struct file_info *fi;
    struct app_io_cq_ring *cring = &s->cq_ring;
    struct io_uring_cqe *cqe;
    unsigned head, reaped = 0;

    head = *cring->head;

    do {
        read_barrier();
        /*
         * Remember, this is a ring buffer. If head == tail, it means that the
         * buffer is empty.
         * */
        if (head == *cring->tail)
            break;

        /* Get the entry */
        cqe = &cring->cqes[head & *s->cq_ring.ring_mask];
        fi = (struct file_info*) cqe->user_data;
        if (cqe->res < 0)
            fprintf(stderr, "Error: %s\n", strerror(abs(cqe->res)));

        int blocks = (int) fi->file_sz / BLOCK_SZ;
        if (fi->file_sz % BLOCK_SZ) blocks++;

        for (int i = 0; i < blocks; i++)
            output_to_console(fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);

        head++;
    } while (1);

    *cring->head = head;
    write_barrier();
}
/*
 * Submit to submission queue.
 * In this function, we submit requests to the submission queue. You can submit
 * many types of requests. Ours is going to be the readv() request, which we
 * specify via IORING_OP_READV.
 *
 * */
int submit_to_sq(char *file_path, struct submitter *s) {
    struct file_info *fi;

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0 ) {
        perror("open");
        return 1;
    }

    struct app_io_sq_ring *sring = &s->sq_ring;
    unsigned index = 0, current_block = 0, tail = 0, next_tail = 0;

    off_t file_sz = get_file_size(file_fd); // 获取文件大小
    if (file_sz < 0)
        return 1;
    off_t bytes_remaining = file_sz;
    int blocks = (int) file_sz / BLOCK_SZ;
    if (file_sz % BLOCK_SZ) blocks++;

    fi = malloc(sizeof(*fi) + sizeof(struct iovec) * blocks);
    if (!fi) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 1;
    }
    fi->file_sz = file_sz;

    /*
     * For each block of the file we need to read, we allocate an iovec struct
     * which is indexed into the iovecs array. This array is passed in as part
     * of the submission. If you don't understand this, then you need to look
     * up how the readv() and writev() system calls work.
     * */
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ)
            bytes_to_read = BLOCK_SZ;

        fi->iovecs[current_block].iov_len = bytes_to_read;

        void *buf;
        if( posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            perror("posix_memalign");
            return 1;
        }
        fi->iovecs[current_block].iov_base = buf;

        current_block++;
        bytes_remaining -= bytes_to_read;
    }

    /* Add our submission queue entry to the tail of the SQE ring buffer */
    next_tail = tail = *sring->tail;
    next_tail++;
    read_barrier();
    index = tail & *s->sq_ring.ring_mask;
    struct io_uring_sqe *sqe = &s->sqes[index];
    sqe->fd = file_fd;
    sqe->flags = 0;
    sqe->opcode = IORING_OP_READV;
    sqe->addr = (unsigned long) fi->iovecs;
    sqe->len = blocks;
    sqe->off = 0; /* offset into file */
    sqe->user_data = (unsigned long long) fi;
    sring->array[index] = index;
    tail = next_tail;

    /* Update the tail so the kernel can see it. */
    if(*sring->tail != tail) {
        *sring->tail = tail;
        write_barrier();
    }

    /*
     * Tell the kernel we have submitted events with the io_uring_enter() system
     * call. We also pass in the IOURING_ENTER_GETEVENTS flag which causes the
     * io_uring_enter() call to wait until min_complete events (the 3rd param)
     * complete.
     * */
    int ret =  io_uring_enter(s->ring_fd, 1,1,
            IORING_ENTER_GETEVENTS);
    if(ret < 0) {
        perror("io_uring_enter");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct submitter *s;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    s = malloc(sizeof(*s));
    if (!s) {
        perror("malloc");
        return 1;
    }
    memset(s, 0, sizeof(*s));

    fd = open(file_path, O_APPEND | O_CREAT | O_WRONLY, 0644);

    if(app_setup_uring(s)) {
        fprintf(stderr, "Unable to setup uring!\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if(submit_to_sq(argv[i], s)) {
            fprintf(stderr, "Error reading file\n");
            return 1;
        }
        read_from_cq(s);
    }

    return 0;
}