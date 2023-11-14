
void io_uring_prep_close(struct io_uring_sqe *sqe, int fd)

void io_uring_prep_close_direct(struct io_uring_sqe *sqe,
					      unsigned file_index)

void io_uring_prep_read(struct io_uring_sqe *sqe, int fd,
				      void *buf, unsigned nbytes, __u64 offset)

void io_uring_prep_write(struct io_uring_sqe *sqe, int fd,
				       const void *buf, unsigned nbytes,
				       __u64 offset)

io_uring_prep_shutdown(struct io_uring_sqe *sqe, int fd,
					  int how);

io_uring_prep_socket_direct(struct io_uring_sqe *sqe,
					       int domain, int type,
					       int protocol,
					       unsigned file_index,
					       unsigned int flags)


void io_uring_prep_connect(struct io_uring_sqe *sqe, int fd,
					 const struct sockaddr *addr,
					 socklen_t addrlen)

void io_uring_buf_ring_add(struct io_uring_buf_ring *br,
					 void *addr, unsigned int len,
					 unsigned short bid, int mask,
					 int buf_offset)

void io_uring_buf_ring_init(struct io_uring_buf_ring *br)

int io_uring_buf_ring_mask(__u32 ring_entries)

struct io_uring_buf_ring *io_uring_setup_buf_ring(struct io_uring *ring,
						  unsigned int nentries,
						  int bgid, unsigned int flags,
						  int *ret);
int io_uring_free_buf_ring(struct io_uring *ring, struct io_uring_buf_ring *br,
			   unsigned int nentries, int bgid);