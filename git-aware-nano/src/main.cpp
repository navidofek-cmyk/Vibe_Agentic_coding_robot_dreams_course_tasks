#include "editor.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Editor editor;
        for (int i = 1; i < argc; ++i)
            editor.openFile(argv[i]);
        editor.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
