#include <string>
#include <inttypes.h>
#include <PLC_app.hpp>

// 	block classes

class __LOCAL__and_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  output0;
    bool  output1;

    void init(){

    }

    void update(){

    }
};

class __LOCAL__not_block{ 
public: 
    bool* input0;
    bool  output0;

    void init(){

    }

    void update(){

    }
};

class __LOCAL__or_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  output0;
    bool  output1;

    void init(){

    }

    void update(){

    }
};

class __LOCAL__int_const_block{ 
public: 
    int  parameter0;
    int64_t  output0;

    void init(){

    }

    void update(){

    }
};

class __LOCAL__bool_const_block{ 
public: 
    bool  parameter0;
    bool  output0;

    void init(){

    }

    void update(){

    }
};

class __LOCAL__double_const_block{ 
public: 
    double  parameter0;
    double  output0;

    void init(){

    }

    void update(){

    }
};



int main(){


// 	block instances


    __LOCAL__and_block block_5;
    __LOCAL__not_block block_6;
    __LOCAL__or_block block_7;
    __LOCAL__int_const_block block_9;
    __LOCAL__bool_const_block block_10;
    __LOCAL__double_const_block block_11;


// 	connections


    block_5.input0 = &block_6.output0;
    block_6.input0 = &block_7.output0;
    block_5.input1 = &block_7.output1;
    block_7.input0 = &block_10.output0;


// 	parameters


    
    block_10.parameter0 = false;
    block_11.parameter0 = 0;


// 	Init blocks


    block_5.init();
    block_6.init();
    block_7.init();
    block_9.init();
    block_10.init();
    block_11.init();


    while(true){

       PLC::LoopStart();
// 	Update blocks


        block_5.update();
        block_6.update();
        block_7.update();
        block_9.update();
        block_10.update();
        block_11.update();

       PLC::LoopEnd();
    }
}


