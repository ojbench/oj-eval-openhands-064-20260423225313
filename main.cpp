#include "printf.h"

int main() {
    sjtu::printf("Hello, %s!\n", "world");
    sjtu::printf("Number: %d\n", 42);
    sjtu::printf("Unsigned: %u\n", 42u);
    sjtu::printf("Escape: %%\n");
    sjtu::printf("Default signed: %_\n", -123);
    sjtu::printf("Default unsigned: %_\n", 456u);
    sjtu::printf("Default string: %_\n", "test");
    std::vector<int> vec = {1, 2, 3};
    sjtu::printf("Vector: %_\n", vec);
    return 0;
}
