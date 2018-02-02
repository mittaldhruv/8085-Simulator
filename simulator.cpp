#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <graphics.h>
#include <conio.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <stack>

using namespace std;

#define delaytime 10
const char *filename = "<current directory>\\codes\\randcode1.txt";

/*
        filenames = {   add_8bit    sub_8bit    1s_comp_8bit    2s_comp_8bit    maskoff_last4bits    maskoff_first4bits

                        shift16bit_left     square      randcode    randcode1   randcode2

                    }
*/
bool step_by_step = false;

string instruction;
string orgaddress;

    // Registers
string A = "00", B = "00", C = "00", D = "00", E = "00", H = "00", L = "00", M = "00";
int S = 0, Z = 0, AC = 0, P = 0, CYY = 0;             // flag registers
string SP = "0000", HL = "0000", PSW = "0000", PC = "0000";
int CCC = 0, IC = 0;

    // For Timing Diagram
int Bytes, MC, TS;


typedef void (*FnPtr)(string );
map<string, FnPtr> functions;
vector<string > codelines;
map<string , string > insopcode;
map<string , string > memadd;
set<string > memaddset;
stack<string > pairstack;
        //  For the timing diagram
vector<string > HObus;
vector<string > LObus;
vector<bool > write;

char hexa_deci[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

int change_char( char X )
{
    for(int i=0;i<=15;i++)
        if( hexa_deci[i] == X )
            return i;
}
char change_int( int X )
{
    for(int i=0;i<=15;i++)
        if( i == X )
            return hexa_deci[i];
}

string nextHex(string s, int changecharidx = 1){
    if( changecharidx <= s.size() ){
        char changechar = s[s.size() - changecharidx];
        if(changechar < 57)
            changechar += 1;
        else if(changechar == 57)
            changechar += 8;
        else if(changechar < 70)
            changechar += 1;
        else if(changechar == 70)
        {
            s = nextHex(s, changecharidx + 1);
            changechar = '0';
        }
        s[s.size() - changecharidx] = changechar;
    }
    return s;
}

        // Function to convert hexadecimal to binary
int hexa_bin(string p,int size)
{
    int rem,i=0,j=1,decimal=0,binary=0;
    while(p[i]!=0)
    {
        if(p[i] > 64)
            p[i]-=55;
        else if(p[i] > 47)
            p[i]-=48;
        decimal += p[i]*pow(16,size);
        i++;
        size--;
    }

    /* At this point, the decimal variable contains corresponding decimal value of that hexadecimal number. */

    while (decimal!=0)
    {
        rem=decimal%2;
        decimal/=2;
        binary+=rem*j;
        j*=10;
    }
    return binary;
}

    //  Function to add two strings and returning the ans in string a
string addhexa(string a, string b, int changecharidx = 1, int carry = 0){
    if( changecharidx <= a.size() ){
        char addbita = a[a.size() - changecharidx];
        char addbitb = b[b.size() - changecharidx];
        int afteraddingbits = change_char(addbita) + change_char(addbitb) + carry;

        a[a.size() - changecharidx] = change_int(afteraddingbits % 16);
        carry = afteraddingbits/16;

        if(changecharidx == a.size() && carry == 1)
            CYY = 1;
        else
            CYY = 0;

        a = addhexa(a, b, changecharidx+1, carry);
    }
    return a;
}
string subhexa(string a, string b, int changecharidx = 1){
    if( changecharidx <= a.size() ){
        char subbita = a[a.size() - changecharidx];
        char subbitb = b[b.size() - changecharidx];
        int aftersubtractingbits;
        if( (change_char(subbita) - change_char(subbitb)) < 0){
            aftersubtractingbits = change_char(subbita) + 16 - change_char(subbitb);
            if(changecharidx == a.size()){
                CYY = 1;
                S = 1;
            }
            else{
                CYY = 0;
                a[a.size() - changecharidx - 1] = change_int( change_char(a[a.size() - changecharidx - 1]) - 1 );
            }

        }
        else{
            aftersubtractingbits = change_char(subbita) - change_char(subbitb);
        }

        a[a.size() - changecharidx] = change_int(aftersubtractingbits % 16);


        a = subhexa(a, b, changecharidx+1);

    }
    return a;
}

void draw_polygon(int x1, int y1, int length, int thickness = 20){
    int slantgap = 10;
    line(x1, y1, x1 + slantgap, y1 - thickness/2);
    line(x1 + slantgap, y1 - thickness/2, x1 + length - slantgap, y1 - thickness/2);
    line(x1 + length - slantgap, y1 - thickness/2, x1 + length, y1);
    line(x1 + length, y1, x1 + length - slantgap, y1 + thickness/2);
    line(x1 + length - slantgap, y1 + thickness/2, x1 + slantgap, y1 + thickness/2);
    line(x1 + slantgap, y1 + thickness/2, x1, y1);
}

void draw_dashline(int x1, int y1, int length){
    int gap = length/9;
    bool counter = true;
    for(int i = x1; i <= x1 + length; i+=gap){
        if(counter){
            line(i, y1, i + gap, y1);
            counter = false;
        }
        else
            counter = true;
    }
}

void printHObus(int x, int y){
    setcolor(WHITE);
    if(!HObus.empty()){
        string temppnt = HObus[0] + "h";
        char *tempprint = (char *)temppnt.c_str();
        HObus.erase(HObus.begin());
        outtextxy(x, y, tempprint);
    }
    setcolor(YELLOW);
}

void printLObus(int x, int y){
    setcolor(WHITE);
    if(!LObus.empty()){
        string temppnt = LObus[0] + "h";
        char *tempprint = (char *)temppnt.c_str();
        LObus.erase(LObus.begin());
        outtextxy(x, y, tempprint);
    }
    setcolor(YELLOW);
}

void display_simulator()
{
    char *printins;
    printins = (char *)instruction.c_str();
    int left = 10, right = 300, top = 25, bottom = 100;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(60,50,"INSTRUCTION  :");
    outtextxy(50,65,"- - - - - - - - - - - - - - - -");
    outtextxy(170,50,printins);


            // Registers Display
    left = 10; right = 300; top = 130; bottom = 690;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(120,140,"REGISTERS");
    outtextxy(110,155,"- - - - - - - - - - - - -");


        // Accumulator and extras
    left = 20; right = 290; top = 180; bottom = 360;
    for(int i = 0; i<5; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 200; i<360; i+=20)
        line(20,i,290,i);
    line(180,180,180,360);
    char *value;
    outtextxy(60,182,"Register"); outtextxy(220,182,"Values");
    value = (char *)A.c_str();
    outtextxy(40,202,"Accumulator"); outtextxy(230,202,value);
    value = (char *)B.c_str();
    outtextxy(40,222,"Register B"); outtextxy(230,222,value);
    value = (char *)C.c_str();
    outtextxy(40,242,"Register C"); outtextxy(230,242,value);
    value = (char *)D.c_str();
    outtextxy(40,262,"Register D"); outtextxy(230,262,value);
    value = (char *)E.c_str();
    outtextxy(40,282,"Register E"); outtextxy(230,282,value);
    value = (char *)H.c_str();
    outtextxy(40,302,"Register H"); outtextxy(230,302,value);
    value = (char *)L.c_str();
    outtextxy(40,322,"Register L"); outtextxy(230,322,value);
    value = (char *)M.c_str();
    outtextxy(40,342,"Memory M"); outtextxy(230,342,value);

        // Flag Registers
    string temp;
    left = 20; right = 290; top = 390; bottom = 510;
    for(int i = 0; i<5; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 410; i<510; i+=20)
        line(20,i,290,i);
    line(180,390,180,510);
    outtextxy(60,392,"Flag Register"); outtextxy(220,392,"Values");
    stringstream ss;
    ss << S;
    ss >> temp;
    value = (char *)temp.c_str();
    outtextxy(40,412,"Sign Flag"); outtextxy(230,412,value);
    stringstream ss1;
    ss1 << Z;
    ss1 >> temp;
    value = (char *)temp.c_str();
    outtextxy(40,432,"Zero Flag"); outtextxy(230,432,value);
    stringstream ss2;
    ss2 << AC;
    ss2 >> temp;
    value = (char *)temp.c_str();
    outtextxy(40,452,"Auxillary Carry"); outtextxy(230,452,value);
    stringstream ss3;
    ss3 << P;
    ss3 >> temp;
    value = (char *)temp.c_str();
    outtextxy(40,472,"Parity"); outtextxy(230,472,value);
    stringstream ss4;
    ss4 << CYY;
    ss4 >> temp;
    ss4.str("");
    value = (char *)temp.c_str();
    outtextxy(40,492,"Carry Flag"); outtextxy(230,492,value);

        // Stack Pointers and extras
    left = 20; right = 290; top = 540; bottom = 680;
    for(int i = 0; i<5; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 560; i<680; i+=20)
        line(20,i,290,i);
    line(180,540,180,680);
    outtextxy(60,542,"Register"); outtextxy(220,542,"Values");
    value = (char *)SP.c_str();
    outtextxy(40,562,"Stack Pointer"); outtextxy(230,562,value);
    value = (char *)HL.c_str();
    outtextxy(40,582,"Memory Pointer"); outtextxy(230,582,value);
    value = (char *)PSW.c_str();
    outtextxy(40,602,"Program Status Word"); outtextxy(230,602,value);
    value = (char *)PC.c_str();
    outtextxy(40,622,"Program Counter"); outtextxy(230,622,value);
    stringstream ss5;
    ss5 << CCC;
    ss5 >> temp;
    ss5.str("");
    value = (char *)temp.c_str();
    outtextxy(40,642,"Clock Cycle Counter"); outtextxy(230,642,value);
    stringstream ss6;
    ss6 << IC;
    ss6 >> temp;
    ss6.str("");
    value = (char *)temp.c_str();
    outtextxy(40,662,"Instruction Counter"); outtextxy(230,662,value);



            // Memory Display
    left = 320; right = 650; top = 25; bottom = 690;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(450,35,"MEMORY");
    outtextxy(440,50,"- - - - - - - - - - -");

    left = 330; right = 480; top = 80; bottom = 680;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 100; i<680; i+=20)
        line(330,i,480,i);
    line(408,80,408,680);
    outtextxy(340,82,"Address"); outtextxy(430,82,"Value");

    left = 490; right = 640; top = 80; bottom = 680;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 100; i<680; i+=20)
        line(490,i,640,i);
    line(568,80,568,680);
    outtextxy(500,82,"Address"); outtextxy(590,82,"Value");


    int outx = 350, outy = 102;
    char *printadd;
    for(set<string > :: iterator it = memaddset.begin(); it != memaddset.end(); ++it){
        string add = *it;
        add += "      |      " + memadd[add];
        printadd = (char *)add.c_str();
        outtextxy(outx,outy,printadd);
        outy += 20;
        if(outy == 682){
            outx = 510;
            outy = 102;
        }
    }


                //  Timing Diagram

    left = 670; right = 1350; top = 25; bottom = 690;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(960,35,"TIMING DIAGRAM");
    outtextxy(950,50,"- - - - - - - - - - - - - - - - - -");

        //  Bold Vertical Line
    left = 730;
    for(int i = 0; i<5; i++, left--)
        line(left,130,left,670);

        // Bold Horizontal Line
    top = 150;
    for(int i = 0; i<5; i++, top--)
        line(710,top,1330,top);

        //  Dividing the x axis according to the no of t-states
    left = 730; right = 1330;
    int tgap = (right - left)/TS;
    int state = 1;
    for(int x = left + tgap; x <= right; x += tgap){

            //  Drawing Vertical Dashed Lines
                int gap = (670-130)/61;
                bool counter = true;
                for(int i = 130; i < 670; i+=gap){
                    if(counter){
                        line(x, i, x, i + gap);
                        counter = false;
                    }
                    else
                        counter = true;
                }

            //  Drawing the clock cycles
        outtextxy(690, 105, "CLK");
        setcolor(GREEN);
        int newx = x-tgap, newy = 120, halfgap = tgap/2;
        line(newx,newy,newx,newy-20);
        line(newx,newy-20,newx+halfgap,newy-20);
        line(newx+halfgap,newy-20,newx+halfgap,newy);
        line(newx+halfgap,newy,newx+tgap,newy);
        setcolor(WHITE);

            //  Naming the t-states
        stringstream ss;
        ss<<"T"<<state;
        string out;
        ss>>out;
        char *printstate = (char *)out.c_str();
        outtextxy(newx + 5, 130, printstate);
        state++;

    }

    string temppnt;
    char *tempprint;

        //  Higher Order BUS
    outtextxy(750, 215, "A15-A8 (Higher Order Address Bus)");
    setcolor(YELLOW);
    int tempTS = TS;

    int startx = 730, starty = 200;
    if(tempTS >= 4 && (tempTS-4)%3==0 ){
        draw_polygon(startx,starty,4*tgap);
            printHObus(startx + 1.5*tgap, starty - 8);
        startx += 4*tgap;
        tempTS -= 4;
        while(tempTS > 0){
            draw_polygon(startx,starty,3*tgap);
                printHObus(startx + 1*tgap, starty - 8);
            startx += 3*tgap;
            tempTS -= 3;
        }
    }
    else{                                           // For cases such as SPHL, PUSH
        while(tempTS > 0){
            draw_polygon(startx,starty,3*tgap);
                printHObus(startx + 1*tgap, starty - 8);
            startx += 3*tgap;
            tempTS -= 3;
        }
    }
    setcolor(WHITE);


        // Lower Order Bus
    outtextxy(750, 275, "DA7-DA0 (Lower Order address/data Bus)");
    setcolor(YELLOW);
    tempTS = TS;

    startx = 730; starty = 260;
    if(tempTS >= 4 && (tempTS-4)%3==0 ){

        draw_polygon(startx,starty,1*tgap);
            printLObus(startx + 0.25*tgap, starty - 8);
        startx += 1*tgap;
        tempTS -= 1;

        draw_polygon(startx,starty,2*tgap);
            printLObus(startx + 0.75*tgap, starty - 8);
        startx += 2*tgap;
        tempTS -= 2;

        draw_dashline(startx,starty,1*tgap);
        startx += 1*tgap;
        tempTS -= 1;
    }
    else if(tempTS >= 6){                                           // For cases such as SPHL, PUSH
        draw_polygon(startx,starty,1*tgap);
            printLObus(startx + 0.25*tgap, starty - 8);
        startx += 1*tgap;
        tempTS -= 1;

        draw_polygon(startx,starty,2*tgap);
            printLObus(startx + 0.75*tgap, starty - 8);
        startx += 2*tgap;
        tempTS -= 2;

        draw_dashline(startx,starty,3*tgap);
        startx += 3*tgap;
        tempTS -= 3;
    }
    while(tempTS > 0){

        draw_polygon(startx,starty,1*tgap);
            printLObus(startx + 0.25*tgap, starty - 8);
        startx += 1*tgap;
        tempTS -= 1;

        draw_polygon(startx,starty,2*tgap);
            printLObus(startx + 0.75*tgap, starty - 8);
        startx += 2*tgap;
        tempTS -= 2;
    }
    setcolor(WHITE);

//-----------------------------------------------------------------------------------------
        //  ALE
    outtextxy(690, 340, "ALE");
    tempTS = TS;

    startx = 730; starty = 360;
    if(tempTS >= 4 && (tempTS-4)%3==0 ){

        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 1*tgap - 10, starty - 30);
        line(startx + 1*tgap - 10, starty - 30, startx + 1*tgap, starty);
        line(startx + 1*tgap, starty, startx + 4*tgap, starty);
        startx += 4*tgap;
        tempTS -= 4;
    }
    else if(tempTS >= 6){                                           // For cases such as SPHL, PUSH
        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 1*tgap - 10, starty - 30);
        line(startx + 1*tgap - 10, starty - 30, startx + 1*tgap, starty);
        line(startx + 1*tgap, starty, startx + 6*tgap, starty);
        startx += 6*tgap;
        tempTS -= 6;
    }
    while(tempTS > 0){

        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 1*tgap - 10, starty - 30);
        line(startx + 1*tgap - 10, starty - 30, startx + 1*tgap, starty);
        line(startx + 1*tgap, starty, startx + 3*tgap, starty);
        startx += 3*tgap;
        tempTS -= 3;
    }

//-----------------------------------------------------------------------------------------
        //  RD
    outtextxy(690, 410, "-----");
    outtextxy(690, 420, "RD");
    tempTS = TS;

    startx = 730; starty = 440;
    if(tempTS >= 4 && (tempTS-4)%3==0 ){

        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 1*tgap, starty - 30);
        line(startx + 1*tgap, starty - 30, startx + 1*tgap + 10, starty);
        line(startx + 1*tgap + 10, starty, startx + 3*tgap - 10, starty);
        line(startx + 3*tgap - 10, starty, startx + 3*tgap, starty - 30);
        line(startx + 3*tgap, starty - 30, startx + 4*tgap, starty - 30);
        startx += 4*tgap;
        tempTS -= 4;
    }
    else if(tempTS >= 6){                                           // For cases such as SPHL, PUSH
        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 1*tgap, starty - 30);
        line(startx + 1*tgap, starty - 30, startx + 1*tgap + 10, starty);
        line(startx + 1*tgap + 10, starty, startx + 3*tgap - 10, starty);
        line(startx + 3*tgap - 10, starty, startx + 3*tgap, starty - 30);
        line(startx + 3*tgap, starty - 30, startx + 6*tgap, starty - 30);
        startx += 6*tgap;
        tempTS -= 6;
    }
    while(tempTS > 0){

        line(startx, starty - 30, startx + 1*tgap, starty - 30);
        if(!write[0]){
            line(startx + 1*tgap, starty - 30, startx + 1*tgap + 10, starty);
            line(startx + 1*tgap + 10, starty, startx + 3*tgap - 10, starty);
            line(startx + 3*tgap - 10, starty, startx + 3*tgap, starty - 30);
        }
        else{
            line(startx + 1*tgap, starty - 30, startx + 3*tgap, starty - 30);
        }
        startx += 3*tgap;
        tempTS -= 3;
        bool temp = write[0];
        write.push_back(temp);
        write.erase(write.begin());
    }

//-----------------------------------------------------------------------------------------
        //  WR
    outtextxy(690, 490, "------");
    outtextxy(690, 500, "WR");
    tempTS = TS;

    startx = 730; starty = 520;
    if(tempTS >= 4 && (tempTS-4)%3==0 ){

        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 4*tgap, starty - 30);
        startx += 4*tgap;
        tempTS -= 4;


    }
    else if(tempTS >= 6){                                           // For cases such as SPHL, PUSH
        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 6*tgap, starty - 30);
        startx += 6*tgap;
        tempTS -= 6;
    }
    while(tempTS > 0){

        line(startx, starty - 30, startx + 1*tgap, starty - 30);
        if(write[0]){
            line(startx + 1*tgap, starty - 30, startx + 1*tgap + 10, starty);
            line(startx + 1*tgap + 10, starty, startx + 3*tgap - 10, starty);
            line(startx + 3*tgap - 10, starty, startx + 3*tgap, starty - 30);
        }
        else{
            line(startx + 1*tgap, starty - 30, startx + 3*tgap, starty - 30);
        }
        startx += 3*tgap;
        tempTS -= 3;
        write.erase(write.begin());
    }

//-----------------------------------------------------------------------------------------
        //  I/O
    outtextxy(690, 570, "     --");
    outtextxy(690, 580, "IO/M");
    tempTS = TS;

    startx = 730; starty = 600;
    if( (tempTS >= 4 && (tempTS-4)%3==0) || tempTS == 6  ){
        line(startx, starty - 30, startx + 10, starty);
        line(startx + 10, starty, startx + tempTS*tgap, starty);
    }
    else{
        line(startx, starty, startx + 10, starty - 30);
        line(startx + 10, starty - 30, startx + 6*tgap - 10, starty - 30);
        line(startx + 6*tgap - 10, starty - 30, startx + 6*tgap, starty);
        startx += 6*tgap;
        tempTS -= 6;
        if(tempTS > 0){
            line(startx, starty, startx + tempTS*tgap, starty);
        }
    }


}


void ORG(string s){
    char ch;
    string add;
    stringstream ss;
    ss<<s;
    ss>>ch>>add>>add;
    add = add.substr(0,4);
    orgaddress = add;
}

void BEGIN(string s){
    char ch;
    string add;
    stringstream ss;
    ss<<s;
    ss>>ch>>add>>add;
    add = add.substr(0,4);
    PC = add;
}

void DB(string s){
    stringstream ss;
    ss<<s;
    char ch;
    string ins;
    ss>>ch>>ins>>ws;            // ws is a whitespace
    string temp;
    while(ss){
        getline(ss,temp,',');
        temp = temp.substr(0,2);
        if(ss){
            memadd[orgaddress] = temp;
            memaddset.insert(orgaddress);
                // for the next Address
            orgaddress = nextHex(orgaddress);
        }
    }
}


/*  ---------------------------------------------------------------------------------------
                                Data Transfer Instructions
                               ---------------------------- */

    //  MOV M,Rs    MOV Rd,Rs   MOV Rd,M
void MOV(string s)
{
	instruction = s;
	stringstream ss;
	ss << s;
	string ins;
	ss >> ins >> ws;
	string temp1,temp2;
	getline(ss,temp1,',');
	temp1 = temp1.substr(0,1);
	ss >> temp2;
	temp2 = temp2.substr(0,1);

        //  MOV M,Rs
	if(temp1 == "M")
	{
	    write.push_back(true);

            // Memory Addressing
		memadd[PC] = insopcode["MOV M, " + temp2];
        memaddset.insert(PC);

                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);

        PC = nextHex(PC);

        string value;
		if(temp2 == "A")
			value = A;
		else if(temp2 == "B")
			value = B;
		else if(temp2 == "C")
			value = C;
		else if(temp2 == "D")
			value = D;
		else if(temp2 == "E")
			value = E;
		else if(temp2 == "H")
			value = H;
		else if(temp2 == "L")
			value = L;

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(value);

        memadd[H+L] = value;
        memaddset.insert(H+L);

            //  Bytes MC TS
        Bytes = 1; MC =2; TS = 7;

	}
	else if(temp2 == "M")
	{
	    write.push_back(false);

        memadd[PC] = insopcode["MOV "+ temp1 + ", M"];
        memaddset.insert(PC);

                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(M);
        PC = nextHex(PC);

		if(temp1 == "A")
			A = memadd[H+L];
		else if(temp1 == "B")
			B = memadd[H+L];
		else if(temp1 == "C")
			C = memadd[H+L];
		else if(temp1 == "D")
			D = memadd[H+L];
		else if(temp1 == "E")
			E = memadd[H+L];
		else if(temp1 == "H")
            H = memadd[H+L];
		else if(temp1 == "L")
            L = memadd[H+L];


            //  Bytes MC TS
        Bytes = 1; MC =2; TS = 7;

	}
	else if(temp1 != "M" && temp2 != "M")
	{

		memadd[PC] = insopcode["MOV "+ temp1 + ", " + temp2];
        memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
        PC = nextHex(PC);

		string value;
		if(temp2 == "A")
			value = A;
		else if(temp2 == "B")
			value = B;
		else if(temp2 == "C")
			value = C;
		else if(temp2 == "D")
			value = D;
		else if(temp2 == "E")
			value = E;
		else if(temp2 == "H")
			value = H;
		else if(temp2 == "L")
			value = L;

		if(temp1 == "A")
			A = value;
		else if(temp1 == "B")
			B = value;
		else if(temp1 == "C")
			C = value;
		else if(temp1 == "D")
			D = value;
		else if(temp1 == "E")
			E = value;
		else if(temp1 == "H")
            H = value;
		else if(temp1 == "L")
            L = value;

            // Bytes MC TS
        Bytes = 1; MC =1; TS = 4;

	}

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //No flags are affected.

}

    //  MVI R, 8-bit Data
void MVI(string s)
{
	instruction = s;
	stringstream ss;
	ss << s;
	string ins, temp1, temp2;
	ss >> ins >> ws;
	getline(ss,temp1,',');
	temp1 = temp1.substr(0,1);
	ss >> temp2;
	temp2 = temp2.substr(0,2);

	memadd[PC] = insopcode["MVI " + temp1 + ", Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = temp2;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(temp1 != "M"){

        write.push_back(false);

        if(temp1 == "A")
            A = temp2;
        else if(temp1 == "B")
            B = temp2;
        else if(temp1 == "C")
            C = temp2;
        else if(temp1 == "D")
            D = temp2;
        else if(temp1 == "E")
            E = temp2;
        else if(temp1 == "H")
            H = temp2;
        else if(temp1 == "L")
            L = temp2;

            // Bytes, MC, TS
        Bytes = 2; MC = 2; TS = 7;
    }
    else{

        write.push_back(false);
        write.push_back(true);

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(M);

        M = temp2;

            // Bytes, MC, TS
        Bytes = 2; MC = 3; TS = 10;
    }

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //No flags are affected.
}

    //  LDA Address
void LDA(string s){
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins, add;
    ss>>ins>>add;
    add = add.substr(0,4);

    write.push_back(false);
    write.push_back(false);
    write.push_back(false);

        // Memory addressing
    memadd[PC] = insopcode["LDA Address"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(2,4);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(0,2);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);

        // coping data from address to the accumulator
    A = memadd[add];

        //  Bytes MC TS
    Bytes = 3; MC = 4; TS = 13;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  LDAX Rp
void LDAX(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,1);

    write.push_back(false);

        //  Memory Addressing
    memadd[PC] = insopcode["LDAX " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back("00");                                      //  complication

    if(add == "B")
        A = memadd[B+C];
    else if(add == "D")
        A = memadd[D+E];

        //  Bytes MC TS
    Bytes = 1; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  LXI Rp, 16-bit data
void LXI(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,Rp,add;
    ss>>ins>>ws;
    getline(ss,Rp,',');
    ss>>add;
    Rp = Rp.substr(0,2);
    add = add.substr(0,4);

    write.push_back(false);
    write.push_back(false);

        //  Memory Addressing
    memadd[PC] = insopcode["LXI " + Rp];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(2,4);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(0,2);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(Rp == "B"){
        B = add.substr(0,2);
        C = add.substr(2,4);
    }
    else if(Rp == "D"){
        D = add.substr(0,2);
        E = add.substr(2,4);
    }
    else if(Rp == "H"){
        H = add.substr(0,2);
        L = add.substr(2,4);
    }
    else if(Rp == "SP"){
        SP = add.substr(0,4);
    }

        //  Bytes MC TS
    Bytes = 3; MC = 3; TS = 10;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //no flags effected
}

    //  LHLD Address
void LHLD(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,4);

    write.push_back(false);
    write.push_back(false);
    write.push_back(false);
    write.push_back(false);

        //  Memory Addressing
    memadd[PC] = insopcode["LHLD Address"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(2,4);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(0,2);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(memadd.count(add))
        L = memadd[add];
    else
        L = "00";

                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);

    add = nextHex(add);

    if(memadd.count(add))
        H = memadd[add];
    else
        H = "00";

                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);


        //  Bytes MC TS
    Bytes = 3; MC = 5; TS = 16;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //no flags effected
}

    //  STA Address
void STA(string s)
{
    instruction = s;
    stringstream ss;
    ss << s;
    string ins,add;
    ss >> ins >> add;
    add = add.substr(0,4);

    write.push_back(false);
    write.push_back(false);
    write.push_back(true);

        // Memory addressing
    memadd[PC] = insopcode["STA Address"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(2,4);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(0,2);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    memadd[add] = A;
    memaddset.insert(add);
                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);

        // Bytes, MC, TS
    Bytes = 3; MC = 4; TS = 13;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //No flags are affected.
}

    // STAX Rp
void STAX(string s)
{
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;

    write.push_back(true);

        // Memory addressing
    memadd[PC] = insopcode["STAX " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string memory;
    if(add == "B")
        memory = B + C;
    else
        memory = D + E;

    memadd[memory] = A;
    memaddset.insert(memory);
                                HObus.push_back(memory.substr(0,2));
                                LObus.push_back(memory.substr(2,4));
                                LObus.push_back(memadd[memory]);

        // Bytes MC TS
    Bytes = 1; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //No flags are affected.
}

    //  SHLD Address
void SHLD(string s)
{
    instruction = s;

    stringstream ss;
    ss << s;
    string ins,add;
    ss >> ins >> add;
    add = add.substr(0,4);

    write.push_back(false);
    write.push_back(false);
    write.push_back(true);
    write.push_back(true);

        // Memory addressing
    memadd[PC] = insopcode["SHLD Address"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(2,4);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add.substr(0,2);
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    memadd[add] = L;
    memaddset.insert(add);
                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);
    add = nextHex(add);
    memadd[add] = H;
    memaddset.insert(add);
                                HObus.push_back(add.substr(0,2));
                                LObus.push_back(add.substr(2,4));
                                LObus.push_back(memadd[add]);

        // Bytes MC TS
    Bytes = 3; MC = 5; TS = 16;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    //No flags are affected.
}

    //  SPHL
void SPHL(string s)
{
    instruction = s;

        // Memory addressing
    memadd[PC] = insopcode["SPHL"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    SP = H;
    SP = SP + L;

        // Bytes MC TS
    Bytes = 1; MC = 1; TS = 6;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    // No flags are affected.
}

    //  XCHG
void XCHG(string s)
{
    instruction = s;

        // Memory addressing
    memadd[PC] = insopcode["XCHG"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string temp1,temp2;
    temp1 = D;
    temp2 = E;
    D = H;
    E = L;
    H = temp1;
    L = temp2;

        // Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    // No flags are affected.
}

    //  PUSH Rp
void PUSH(string s)
{
    instruction = s;

    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;

    write.push_back(true);
    write.push_back(true);

        // Memory addressing
    memadd[PC] = insopcode["PUSH " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add == "B")
        pairstack.push(B+C);
    else if(add == "D")
        pairstack.push(D+E);
    else if(add == "H")
        pairstack.push(H+L);
    else if(add == "PSW")
        pairstack.push(PSW);

        // Bytes MC TS
    Bytes = 1; MC = 3; TS = 12;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    // No flags are affected.
}

    //  POP Rp
void POP(string s)
{
    instruction = s;

    stringstream ss;
    ss << s;
    string ins,add;
    ss >> ins >> add;

    write.push_back(false);
    write.push_back(false);

        // Memory addressing
    memadd[PC] = insopcode["POP " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back("00");                                      //  complication
    string tempPC = nextHex(PC);
                                HObus.push_back(tempPC.substr(0,2));
                                LObus.push_back(tempPC.substr(2,4));
                                LObus.push_back("00");                                      //  complication

    string data = pairstack.top();
    pairstack.pop();

    if(add == "B"){
        B = data.substr(0,2);
        C = data.substr(2,4);
    }
    else if(add == "D"){
        D = data.substr(0,2);
        E = data.substr(2,4);
    }
    else if(add == "H"){
        H = data.substr(0,2);
        L = data.substr(2,4);
    }
    else if(add == "PSW")
        PSW = data;

        // Bytes MC TS
    Bytes = 1; MC = 3; TS = 10;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

    // No flags are affected.
}
/*  ---------------------------------------------------------------------------------------
                                Arithmatic Instructions
                               ---------------------- */

    //  ADD R/M
void ADD(string s){

    instruction = s;
    stringstream ss;
    ss<<s;
    string ins, add;
    ss>>ins>>add;
    add = add.substr(0,1);

        // Memory Addressing
    memadd[PC] = insopcode["ADD " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){

        string value;
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

        A = addhexa(A, value);          //  Updating the value of Accumulator

            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(M);

        A = addhexa(A, M);          //  Updating the value of Accumulator

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;

    }

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  ADC R/M
void ADC(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins, add;
    ss>>ins>>add;
    add = add.substr(0,1);

        // Memory Addressing
    memadd[PC] = insopcode["ADC " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){

        string value;
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

        A = addhexa(A, value);          //  Updating the value of Accumulator
        A = addhexa(A, "00", 1, CYY);

            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(M);

        A = addhexa(A, M);          //  Updating the value of Accumulator
        A = addhexa(A, "00", 1, CYY);

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;

    }

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // ADI 8-bit Data
void ADI( string s )
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

    memadd[PC] = insopcode["ADI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    A = addhexa(A, add);

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

}

    // ACI 8-bit Data
void ACI( string s )
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

    memadd[PC] = insopcode["ACI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    A = addhexa(A, add);
    A = addhexa(A, "00", 1, CYY);

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

}

    //  SUB R/M
void SUB(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins, add;
    ss>>ins>>add;
    add = add.substr(0,1);

        // Memory Addressing
    memadd[PC] = insopcode["SUB " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){

        string value;
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

        A = subhexa(A, value);          //  Updating the value of Accumulator

            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        A = subhexa(A, M);          //  Updating the value of Accumulator

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;

    }

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  SBB R/M
void SBB(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins, add;
    ss>>ins>>add;
    add = add.substr(0,1);

        // Memory Addressing
    memadd[PC] = insopcode["SBB " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){

        string value;
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

        if(CYY == 1){
            A = subhexa(A, value);          //  Updating the value of Accumulator
            A = subhexa(A, "01");
        }
        else
            A = subhexa(A, value);


            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        if(CYY == 1){
            A = subhexa(A, M);          //  Updating the value of Accumulator
            A = subhexa(A, "01");
        }
        else
            A = subhexa(A, M);

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;

    }

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  SUI 8-bit Data
void SUI(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

    memadd[PC] = insopcode["SUI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    A = subhexa(A, add);

        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  SBI 8-bit Data
void SBI(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

    memadd[PC] = insopcode["SBI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(CYY == 1){
        A = subhexa(A, add);
        A = subhexa(A, "01");
    }
    else
        A = subhexa(A, add);


        //  Parity flag
    if( change_char(A[A.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( A == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  DAD Rp
void DAD( string s )
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,Rp,temp; // temp is to realize the register pair from given Rp
    ss>>ins>>Rp;

    memadd[PC] = insopcode["DAD " + Rp];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if( Rp == "B" )
        temp = B+C;
    else if( Rp == "D" )
        temp = D+E;
    else if( Rp == "H" )
        temp = H+L;
    else if( Rp == "SP" )
        temp = SP;

    temp = addhexa(H+L, temp);
    H = temp.substr(0,2);
    L = temp.substr(2,4);

        //  Bytes MC TS
    Bytes = 1; MC = 3; TS = 10;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;

}

    //  DCR R/M
void DCR(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,1);

        //  Memory Addressing
    memadd[PC] = insopcode[ins + " " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value;

    if(add != "M")
    {

        if(add == "A"){
            A = subhexa(A, "01");
            value = A;
        }
        else if(add == "B"){
            B = subhexa(B, "01");
            value = B;
        }
        else if(add == "C"){
            C = subhexa(C, "01");
            value = C;
        }
        else if(add == "D"){
            D = subhexa(D, "01");
            value = D;
        }
        else if(add == "E"){
            E = subhexa(E, "01");
            value = E;
        }
        else if(add == "H"){
            H = subhexa(H, "01");
            value = H;
        }
        else if(add == "L"){
            L = subhexa(L, "01");
            value = L;
        }


            //  Bytes, MC, TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                    HObus.push_back(PC.substr(0,2));
                                    LObus.push_back(PC.substr(2,4));
                                    LObus.push_back("00");

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        memadd[H+L] = subhexa(memadd[H+L], "01");
        value = memadd[H+L];

            //  Bytes, MC, TS
        Bytes = 1; MC = 3; TS = 10;

    }

        //  Parity flag
    if( change_char(value[value.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( value == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  DCX Rp
void DCX(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

        //  Memory Addressing
    memadd[PC] = insopcode[ins + " " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value;

    if(add == "B"){
        value = subhexa(B+C, "0001");
        B = value.substr(0,2);
        C = value.substr(2,4);
    }
    else if(add == "D"){
        value = subhexa(D+E, "0001");
        D = value.substr(0,2);
        E = value.substr(2,4);
    }
    else if(add == "H"){
        value = subhexa(H+L, "0001");
        H = value.substr(0,2);
        L = value.substr(2,4);
    }
    else if(add == "SP"){
        value = subhexa(SP, "0001");
        SP = value;
    }

        //  Bytes, MC, TS
    Bytes = 1; MC = 1; TS = 6;

        //  Parity flag
    if( change_char(value[value.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( value == "0000" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  INR R/M
void INR(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,1);

        //  Memory Addressing
    memadd[PC] = insopcode[ins + " " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value;

    if(add != "M")
    {

        if(add == "A"){
            A = addhexa(A, "01");
            value = A;
        }
        else if(add == "B"){
            B = addhexa(B, "01");
            value = B;
        }
        else if(add == "C"){
            C = addhexa(C, "01");
            value = C;
        }
        else if(add == "D"){
            D = addhexa(D, "01");
            value = D;
        }
        else if(add == "E"){
            E = addhexa(E, "01");
            value = E;
        }
        else if(add == "H"){
            H = addhexa(H, "01");
            value = H;
        }
        else if(add == "L"){
            L = addhexa(L, "01");
            value = L;
        }


            //  Bytes, MC, TS
        Bytes = 1; MC = 1; TS = 4;

    }
    else if(add == "M"){

                                    HObus.push_back(PC.substr(0,2));
                                    LObus.push_back(PC.substr(2,4));
                                    LObus.push_back("00");

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        memadd[H+L] = addhexa(memadd[H+L], "01");
        value = memadd[H+L];

            //  Bytes, MC, TS
        Bytes = 1; MC = 3; TS = 10;

    }

        //  Parity flag
    if( change_char(value[value.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( value == "00" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  INX Rp
void INX(string s)
{
    instruction = s;
    stringstream ss;
    ss<<s;
    string ins,add;
    ss>>ins>>add;
    add = add.substr(0,2);

        //  Memory Addressing
    memadd[PC] = insopcode[ins + " " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value;

    if(add == "B"){
        value = addhexa(B+C, "0001");
        B = value.substr(0,2);
        C = value.substr(2,4);
    }
    else if(add == "D"){
        value = addhexa(D+E, "0001");
        D = value.substr(0,2);
        E = value.substr(2,4);
    }
    else if(add == "H"){
        value = addhexa(H+L, "0001");
        H = value.substr(0,2);
        L = value.substr(2,4);
    }
    else if(add == "SP"){
        value = addhexa(SP, "0001");
        SP = value;
    }

        //  Bytes, MC, TS
    Bytes = 1; MC = 1; TS = 6;

        //  Parity flag
    if( change_char(value[value.size() - 1])%2 != 0 )       //  Checking if the no is odd, by checking the last character of the string
        P = 1;              //  Odd
    else
        P = 0;              //  Even

        //  Zero Flag
    if( value == "0000" ) // Zero flag
        Z = 1;              //  If accumulator is 00
    else
        Z = 0;              // If accumulator is non zero

        //  Sign flag
    S = 0;                  //  If the accumulator is positive


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}


/*  ---------------------------------------------------------------------------------------
                                Logical Instructions
                               ---------------------- */

    // CMP R/M
void CMP(string s){
    instruction = s;

    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;       //cascading.
    add = add.substr(0,1);

        //  Memory Addressing
    memadd[PC] = insopcode[ins + " " + add];
    memaddset.insert(PC);
                            HObus.push_back(PC.substr(0,2));
                            LObus.push_back(PC.substr(2,4));
                            LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){

        string value;
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

        if(A < value){
            CYY = 1;
            Z = 0;
        }
        else if(A == value){
            CYY = 0;
            Z = 1;
        }
        else if(A > value){
            CYY = 0;
            Z = 0;
        }
            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;
    }
    else if(add == "M"){

                                HObus.push_back(H);
                                LObus.push_back(L);
                                LObus.push_back(M);

        if(A < M){
            CYY = 1;
            Z = 0;
        }
        if(A == M){
            CYY = 0;
            Z = 1;
        }
        if(A > M){
            CYY = 0;
            Z = 0;
        }

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;
    }


        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // CPI 8-bit Data
void CPI(string s){
    instruction  = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,2);

    memadd[PC] = insopcode["CPI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(A < add){
        CYY = 1;
        Z = 0;
    }
    else if(A == add){
        CYY = 0;
        Z = 1;
    }
    else if(A > add){
        CYY = 0;
        Z = 0;
    }

        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // ANA R/M
void ANA(string s){
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,1);

    string value;

    memadd[PC] = insopcode["ANA " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

            //  Bytes MC TS
        Bytes = 1; MC = 1; TS = 4;
    }
    else if(add == "M"){

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        value =  M;

            //  Bytes MC TS
        Bytes = 1; MC = 2; TS = 7;
    }

    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(value, value.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )&( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    CYY = 0;
    AC = 1;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // ANI 8-bit Data
void ANI(string s){
    instruction  = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,2);

        // Memory Addressing
    memadd[PC] = insopcode["ANI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(add, add.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )&( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    CYY = 0;
    AC = 1;

        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // ORA R/M
void ORA(string s){
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,1);

    string value;
        // Memory addressing
    memadd[PC] = insopcode["ORA " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){
        if(add == "A")
            value = A;
        else if(add == "B")
            value = B;
        else if(add == "C")
            value = C;
        else if(add == "D")
            value = D;
        else if(add == "E")
            value = E;
        else if(add == "H")
            value = H;
        else if(add == "L")
            value = L;

            // Bytes, MC, TS
        Bytes = 1; MC = 1; TS = 4;
    }
    else if(add == "M"){

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        value = M;

            // Bytes, MC, TS
        Bytes = 1; MC = 2; TS = 7;
    }

    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(value, value.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )|( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    AC = 0;
    CYY = 0;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // ORI 8-bit Data
void ORI(string s){
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,2);

        // Memory Addressing
    memadd[PC] = insopcode["ORI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value = add;
    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(value, value.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )|( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    AC = 0;
    CYY = 0;

        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // XRA R/M
void XRA(string s){
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,1);

    string value;
             // Memory addressing
    memadd[PC] = insopcode["XRA " + add];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    if(add != "M"){
        if(add == "A")
            value = A;
        else if(add == "B")
          value = B;
        else if(add == "C")
          value = C;
        else if(add == "D")
          value = D;
        else if(add == "E")
          value = E;
        else if(add == "H")
          value = H;
        else if(add == "L")
          value = L;

            // Bytes, MC, TS
        Bytes = 1; MC = 1; TS = 4;
    }
    else if(add == "M"){

                                    HObus.push_back(H);
                                    LObus.push_back(L);
                                    LObus.push_back(M);

        value = memadd[add];

            // Bytes, MC, TS
        Bytes = 1; MC = 2; TS = 7;
    }

    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(value, value.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )^( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    AC = 0;
    CYY = 0;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // XRI 8-bit Data
void XRI(string s){
    instruction = s;
    stringstream ss;
    ss << s;
    string ins, add;
    ss >> ins >> add;
    add = add.substr(0,2);

        // Memory Addressing
    memadd[PC] = insopcode["XRI Data"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);
    memadd[PC] = add;
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    string value = add;
    int val1 = hexa_bin(A, A.size() - 1);
    int val2 = hexa_bin(value, value.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10, val2 /= 10)
        ans += ( ( val1%10 )^( val2%10 ) )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

    AC = 0;
    CYY = 0;

        //  Bytes MC TS
    Bytes = 2; MC = 2; TS = 7;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  CMA
void CMA(string s){
    instruction = s;

        // Memory Addressing
    memadd[PC] = insopcode["CMA"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    int val1 = hexa_bin(A, A.size() - 1);

    int ans = 0;
    for(int i = 0; i<8; i++, val1 /= 10)
        ans += ( ( val1%10 )|0 )*pow(2,i);

    stringstream ss1;
    ss1<<hex<<uppercase<<ans;

    ss1>>A;                 // Value of Accumulator is updated.

        //  Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // CMC
void CMC(string s){
    instruction = s;

        // Memory Addressing
    memadd[PC] = insopcode["CMC"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    CYY = ~CYY;

        //  Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    //  STC
void STC(string s){

    instruction = s;

        // Memory Addressing
    memadd[PC] = insopcode["CMC"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    CYY = 1;

        //  Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}


/*  ---------------------------------------------------------------------------------------
                                Control Instructions
                               ---------------------- */

    // NOP
void NOP(string s){

    instruction = s;

        // Memory Addressing
    memadd[PC] = insopcode["NOP"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

    delay(1000);

        //  Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

    // HLT
void HLT(string s){
    instruction = s;

        // Memory Addressing
    memadd[PC] = insopcode["HLT"];
    memaddset.insert(PC);
                                HObus.push_back(PC.substr(0,2));
                                LObus.push_back(PC.substr(2,4));
                                LObus.push_back(memadd[PC]);
    PC = nextHex(PC);

        //  Bytes MC TS
    Bytes = 1; MC = 1; TS = 4;

        // Clock Cycles
    CCC += TS;

        // Instruction Counter
    IC++;
}

//  ---------------------------------------------------------------------------------------


void storeFunctionsMap()
{
            //  Hashed Instructions
    functions["ORG"] = ORG;
    functions["BEGIN"] = BEGIN;
    functions["DB"] = DB;

            //  Data Transfer Instructions
    functions["MOV"] = MOV;
    functions["MVI"] = MVI;
    functions["LDA"] = LDA;
    functions["LDAX"] = LDAX;
    functions["LXI"] = LXI;
    functions["LHLD"] = LHLD;
    functions["STA"] = STA;
    functions["STAX"] = STAX;
    functions["SHLD"] = SHLD;
    functions["SPHL"] = SPHL;
    functions["XCHG"] = XCHG;
    functions["PUSH"] = PUSH;
    functions["POP"] = POP;

            //  Arithematic Instructions
    functions["ADD"] = ADD;
    functions["ADC"] = ADC;
    functions["ADI"] = ADI;
    functions["ACI"] = ACI;
    functions["SUB"] = SUB;
    functions["SBB"] = SBB;
    functions["SUI"] = SUI;
    functions["SBI"] = SBI;
    functions["DAD"] = DAD;
    functions["DCR"] = DCR;
    functions["DCX"] = DCX;
    functions["INR"] = INR;
    functions["INX"] = INX;

            //  Logical Instructions
    functions["CMA"] = CMA;
    functions["CMC"] = CMC;
    functions["STC"] = STC;
    functions["CMP"] = CMP;
    functions["CPI"] = CPI;
    functions["ANA"] = ANA;
    functions["ANI"] = ANI;
    functions["ORA"] = ORA;
    functions["ORI"] = ORI;
    functions["XRA"] = XRA;
    functions["XRI"] = XRI;

            // Control Instructions
    functions["NOP"] = NOP;
    functions["HLT"] = HLT;
}

void storeInsopcodeMap()
{

            //  Data Transfer Instructions
    insopcode["MOV A, A"] = "7F";
    insopcode["MOV A, B"] = "78";
    insopcode["MOV A, C"] = "79";
    insopcode["MOV A, D"] = "7A";
    insopcode["MOV A, E"] = "7B";
    insopcode["MOV A, H"] = "7C";
    insopcode["MOV A, L"] = "7D";
    insopcode["MOV A, M"] = "7E";
    insopcode["MOV B, A"] = "47";
    insopcode["MOV B, B"] = "40";
    insopcode["MOV B, C"] = "41";
    insopcode["MOV B, D"] = "42";
    insopcode["MOV B, E"] = "43";
    insopcode["MOV B, H"] = "44";
    insopcode["MOV B, L"] = "45";
    insopcode["MOV B, M"] = "46";
    insopcode["MOV C, A"] = "4F";
    insopcode["MOV C, B"] = "48";
    insopcode["MOV C, C"] = "49";
    insopcode["MOV C, D"] = "4A";
    insopcode["MOV C, E"] = "4B";
    insopcode["MOV C, H"] = "4C";
    insopcode["MOV C, L"] ="4D";
    insopcode["MOV C, M"] = "4E";
    insopcode["MOV D, A"] = "57";
    insopcode["MOV D, B"] = "50";
    insopcode["MOV D, C"] = "51";
    insopcode["MOV D, D"] = "52";
    insopcode["MOV D, E"] = "53";
    insopcode["MOV D, H"] = "54";
    insopcode["MOV D, L"] = "55";
    insopcode["MOV D, M"] = "56";
    insopcode["MOV E, A"] = "5F";
    insopcode["MOV E, B"] = "58";
    insopcode["MOV E, C"] = "59";
    insopcode["MOV E, D"] = "5A";
    insopcode["MOV E, E"] = "5B";
    insopcode["MOV E, H"] = "5C";
    insopcode["MOV E, L"] = "5D";
    insopcode["MOV E, M"] = "5E";
    insopcode["MOV H, A"] = "67";
    insopcode["MOV H, B"] = "60";
    insopcode["MOV H, C"] = "61";
    insopcode["MOV H, D"] = "62";
    insopcode["MOV H, E"] = "63";
    insopcode["MOV H, H"] = "64";
    insopcode["MOV H, L"] = "65";
    insopcode["MOV H, M"] = "66";
    insopcode["MOV L, A"] = "6F";
    insopcode["MOV L, B"] = "68";
    insopcode["MOV L, C"] = "69";
    insopcode["MOV L, D"] = "6A";
    insopcode["MOV L, E"] = "6B";
    insopcode["MOV L, H"] = "6C";
    insopcode["MOV L, L"] = "6D";
    insopcode["MOV L, M"] = "6E";
    insopcode["MOV M, A"] = "77";
    insopcode["MOV M, B"] = "70";
    insopcode["MOV M, C"] = "71";
    insopcode["MOV M, D"] = "72";
    insopcode["MOV M, E"] = "73";
    insopcode["MOV M, H"] = "74";
    insopcode["MOV M, L"] = "75";
    insopcode["MVI A, Data"] = "3E";
    insopcode["MVI B, Data"] = "06";
    insopcode["MVI C, Data"] = "0E";
    insopcode["MVI D, Data"] = "16";
    insopcode["MVI E, Data"] = "1E";
    insopcode["MVI H, Data"] = "26";
    insopcode["MVI L, Data"] = "2E";
    insopcode["MVI M, Data"] = "36";
    insopcode["LDA Address"] = "3A";
    insopcode["LDAX B"] = "0A";
    insopcode["LDAX D"] = "1A";
    insopcode["LXI B"] = "01";
    insopcode["LXI D"] = "11";
    insopcode["LXI H"] = "21";
    insopcode["LXI SP"] = "31";
    insopcode["LHLD Address"] = "2A";
    insopcode["STA Address"] = "32";
    insopcode["STAX B"] = "02";
    insopcode["STAX D"] = "12";
    insopcode["SHLD Address"] = "22";
    insopcode["SPHL"] = "F9";
    insopcode["XCHG"] = "EB";
    insopcode["PUSH B"] = "C5";
    insopcode["PUSH D"] = "D5";
    insopcode["PUSH H"] = "E5";
    insopcode["PUSH PSW"] = "F5";
    insopcode["POP B"] = "C1";
    insopcode["POP D"] = "D1";
    insopcode["POP H"] = "E1";
    insopcode["POP PSW"] = "F1";

            //  Arithematic Instructions
    insopcode["ADD A"] = "87";
    insopcode["ADD B"] = "80";
    insopcode["ADD C"] = "81";
    insopcode["ADD D"] = "82";
    insopcode["ADD E"] = "83";
    insopcode["ADD H"] = "84";
    insopcode["ADD L"] = "85";
    insopcode["ADD M"] = "86";
    insopcode["ADC A"] = "8F";
    insopcode["ADC B"] = "88";
    insopcode["ADC C"] = "89";
    insopcode["ADC D"] = "8A";
    insopcode["ADC E"] = "8B";
    insopcode["ADC H"] = "8C";
    insopcode["ADC L"] = "8D";
    insopcode["ADC M"] = "8E";
    insopcode["ADI Data"] = "C6";
    insopcode["ACI Data"] = "CE";
    insopcode["DAD B"] = "09";
    insopcode["DAD D"] = "19";
    insopcode["DAD H"] = "29";
    insopcode["DAD SP"] = "39";
    insopcode["SUB A"] = "97";
    insopcode["SUB B"] = "90";
    insopcode["SUB C"] = "91";
    insopcode["SUB D"] = "92";
    insopcode["SUB E"] = "93";
    insopcode["SUB H"] = "94";
    insopcode["SUB L"] = "95";
    insopcode["SUB M"] = "96";
    insopcode["SBB A"] = "9F";
    insopcode["SBB B"] = "98";
    insopcode["SBB C"] = "99";
    insopcode["SBB D"] = "9A";
    insopcode["SBB E"] = "9B";
    insopcode["SBB H"] = "9C";
    insopcode["SBB L"] = "9D";
    insopcode["SBB M"] = "9E";
    insopcode["SUI Data"] = "D6";
    insopcode["SBI Data"] = "DE";
    insopcode["INR A"] = "3C";
    insopcode["INR B"] = "04";
    insopcode["INR C"] = "0C";
    insopcode["INR D"] = "I4";
    insopcode["INR E"] = "IC";
    insopcode["INR H"] = "24";
    insopcode["INR L"] = "2C";
    insopcode["INR M"] = "34";
    insopcode["INX B"] = "03";
    insopcode["INX D"] = "I3";
    insopcode["INX H"] = "23";
    insopcode["INX SP"] = "33";
    insopcode["DCR A"] = "3D";
    insopcode["DCR B"] = "05";
    insopcode["DCR C"] = "0D";
    insopcode["DCR D"] = "15";
    insopcode["DCR E"] = "1D";
    insopcode["DCR H"] = "25";
    insopcode["DCR L"] = "2D";
    insopcode["DCR M"] = "35";
    insopcode["DCX B"] = "0B";
    insopcode["DCX D"] = "1B";
    insopcode["DCX H"] = "2B";
    insopcode["DCX SP"] = "3B";
    insopcode["DAA"] = "27";

            //  Branching Instructions
    insopcode["JC M"]= "DA";
    insopcode["JM M"]= "FA";
    insopcode["JMP M"]= "C3";
    insopcode["JNC M"]= "D2";
    insopcode["JNZ M"]= "C2";
    insopcode["JP M"]= "F2";
    insopcode["JPE M"]= "EA";
    insopcode["JPO M"]= "E2";
    insopcode["JZ M"]= "CA";

            //  Logical Instructions
    insopcode["CMA"] = "2F";
    insopcode["CMC"] = "3F";
    insopcode["STC"] = "37";
    insopcode["CMP A"] = "BF";
    insopcode["CMP B"] = "B8";
    insopcode["CMP C"] = "B9";
    insopcode["CMP D"] = "BA";
    insopcode["CMP E"] = "BB";
    insopcode["CMP H"] = "BC";
    insopcode["CMP L"] = "BD";
    insopcode["CMP M"] = "BE";
    insopcode["CPI Data"] = "FE";
    insopcode["ANA A"] = "A7";
    insopcode["ANA B"] = "A0";
    insopcode["ANA C"] = "A1";
    insopcode["ANA D"] = "A2";
    insopcode["ANA E"] = "A3";
    insopcode["ANA H"] = "A4";
    insopcode["ANA L"] = "A5";
    insopcode["ANA M"] = "A6";
    insopcode["ANI Data"] = "E6";
    insopcode["ORA A"] = "B7";
    insopcode["ORA B"] = "B0";
    insopcode["ORA C"] = "B1";
    insopcode["ORA D"] = "B2";
    insopcode["ORA E"] = "B3";
    insopcode["ORA H"] = "B4";
    insopcode["ORA L"] = "B5";
    insopcode["ORA M"] = "B6";
    insopcode["ORI Data"] = "F6";
    insopcode["XRA A"] = "AF";
    insopcode["XRA B"] = "A8";
    insopcode["XRA C"] = "A9";
    insopcode["XRA D"] = "AA";
    insopcode["XRA E"] = "AB";
    insopcode["XRA H"] = "AC";
    insopcode["XRA L"] = "AD";
    insopcode["XRA M"] = "AE";
    insopcode["XRI Data"] = "EE";

            // Control Instructions
    insopcode["NOP"] = "00";
    insopcode["HLT"] = "76";
}

void readFile()
{
    string s;
    ifstream fin(filename);
    while(!fin.eof()){
        getline(fin,s,'\n');
        if(s.substr(0,1) == "#"){
            stringstream ss;
            ss<<s;
            char ch;
            string ins;
            ss>>ch>>ins;
            if(functions.count(ins))
                functions[ins] (s);
        }
        else
            codelines.push_back(s);
    }
    fin.close();
}
void execute(){
    int maxlineno = codelines.size();
    for(int i = 0; i<maxlineno; i++){
        string s,temp;
        s = codelines[i];
        if(!s.size())
            continue;
        stringstream ss;
        ss<<s;
        if( s.find(':') && ( s.find(':') < s.size() ) ){                                    //  complication
            s = s.substr(s.find(':')+1, s.size());
            getline(ss,temp,':');
            ss>>ws;
        }

        char ch;
        string ins;
        ss>>ins;
        if(functions.count(ins)){
            functions[ins] (s);

            if(memadd.count(H+L))
                M = memadd[H+L];
            else
                M = "00";

                // Calling the display function after every instruction
            cleardevice();
            display_simulator();
                //  For Clearing the bus
            HObus.clear();
            LObus.clear();
            write.clear();
        }


                //  Checking if step by step or run all
        if(step_by_step){
            while(true){
                if(GetAsyncKeyState(VK_RETURN))
                    break;
                if(GetAsyncKeyState(VK_ESCAPE))
                    exit(0);
            }
        }
        else
            delay(delaytime);


    }
}


int main(){
    int gdriver = DETECT, gmode;       // requests auto detection.
    initgraph(&gdriver,&gmode,"");
    initwindow(1360,700);
    setcolor(WHITE);


    storeFunctionsMap();
    storeInsopcodeMap();

    cleardevice();
    int left = 400, right = 900, top = 200, bottom = 500;
    for(int i = 0; i<10; i++, left--, right++, top--, bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(600,220,"8085 Simulator");
    outtextxy(500,250,"-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-");
    outtextxy(500, 300, "Choose one of the buttons :");

    left = 450; right = 640; top = 350; bottom = 450;
    for(int i = 0; i<5; i++, left--, right++, top--, bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(470,370,"Press Spacebar to - ");
    outtextxy(470,400,"Run All at a Time");

    left = 660; right = 850; top = 350; bottom = 450;
    for(int i = 0; i<5; i++, left--, right++, top--, bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(680,370,"Press Enter to - ");
    outtextxy(680,400,"Run Step By Step");

    while(true){
        if(GetAsyncKeyState(VK_SPACE)){
            step_by_step = false;
            break;
        }
        if(GetAsyncKeyState(VK_RETURN)){
            step_by_step = true;
            break;
        }
        if(GetAsyncKeyState(VK_ESCAPE))
            exit(0);
    }
    cleardevice();

    readFile();
    execute();


    system("PAUSE");
    return 0;
}
