#include <unistd.h>
#include "webserver.h"

int main() {
    Webserver server(1316, 3, 60000, false, 4);
    server.Start();
}