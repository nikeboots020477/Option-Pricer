#include "control.h"

int main(int argc, char *argv[]){

try{
    Request request(argc, argv);
    Controller controller;
    controller.handle_request(request);
}
catch(MyException()){
    std::cout<<"ERRORRR";
}



//Controller controller;

//controller.handle_request(request);

return 0;
}