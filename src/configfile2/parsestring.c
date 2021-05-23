#include "configfile2/parsestring.h"
#include <stdlib.h>

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code

char* ReadInteger(char* readptr,int* u){
    char c=*readptr;
    int value=0;
    bool sign=false;
    if (c=='-'){
        sign=true;
        c=*++readptr;
    }else if (c=='+'){
        c=*++readptr;
    }
    while (c>='0' && c<='9'){
        value*=10;
        value+=c-'0';
        c=*++readptr;
    }
    if (sign) value=-value;
    *u=value;
    return readptr;
}

char* SkipBlankSpace(char* s){ //skip tabulation and space
    for (char c;(c=*s)==' ' || c=='\t';s++);
    return s;
}

char* SkipBlankSpaceReverse(char* s){ //skip tabulation and space
    for (char c;(c=*s)==' ' || c=='\t';s--);
    return s;
}

char* RstripString(char* s,char d){
    char* r=NULL;
    for (char c;(c=*s++);){
        if (c==d || c=='\n'){
            r=s;
            break;
        }
    }
    *(SkipBlankSpaceReverse(s-2)+1)='\0';
    return r;
}

char* DelimitString(char* s,char d){
    for (char c;(c=*s);s++){
        if (c==d){
            *s++='\0';
            break;
        }
    }
    return s;
}

char* SkipLine(char* s){
    for(char c;(c=*s++);){
        if (c=='\n') return s;
        if (c=='\r'){ //CRLF
            if (*s=='\n') s++;
            return s;
        }
    }
    return NULL;
}

char* ReadNumber(char* read_ptr,void* ptr,bool* isdouble){
    double double_value=0;
    char cc;
    int e=0,e1=0;
    bool exponent_sign,sign=false,type=false;

    if (*read_ptr=='-'){
        sign=true;
        read_ptr++;
    }
    while((cc=*read_ptr)>='0' && cc<='9'){
        double_value*=10;
        double_value+=cc-'0';
        read_ptr++;
    }
    if  (cc=='.'){
        type=true;
        while ((cc=*++read_ptr)>='0' && cc<='9'){
           double_value*=10.0;
           double_value+=cc-'0';
           e--;
        }      
    }
    if (cc=='e' || cc=='E'){
        cc=*++read_ptr;
        exponent_sign=false;
        if (cc=='-'){
            type=true;
            cc=*++read_ptr;
            exponent_sign=true;
        }
        if (cc=='+') cc=*++read_ptr;
        
        while (cc>='0' && cc<='9'){
            e1*=10;
            e1+=cc-'0';
            cc=*++read_ptr;
        }
        if (exponent_sign) e1=-e1;
        e+=e1;
    }
    if (e){
        if (e<0){
            while (e){
                double_value/=10.0;
                e++;
            }
        }else{
            while (e){
                double_value*=10.0;
                e--;
            }
        }
    }
    if (sign) double_value=-double_value;
    if (type){
        *(double*)ptr=double_value;
        *isdouble=true;
    }else{

        *(int64_t*)ptr=(int64_t)double_value;
        *isdouble=false;
    }
    return read_ptr;
}
