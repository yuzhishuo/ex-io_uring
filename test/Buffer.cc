#include <unifex/detail/intrusive_list.hpp>
struct Buffer {
public:
  Buffer *root; // TODO: need it?
  Buffer *next;
  Buffer *prev;

private:
};

int main(int argc, char const *argv[]) {

  unifex::intrusive_list<Buffer, &Buffer::next, &Buffer::prev> f;

  
  /* code */
  return 0;
}
