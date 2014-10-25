#include <iostream>
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>
#include <vector>
using namespace std;

int main(){
srand(time(NULL));

int a ;
int b ;
vector<int> A;
vector<int> B;
int tempa;
int tempb; 
//int tempt;
bool flag;
float c ;
int np= 300;
int i;
int EndTime;

cout << "[nodes]"<<endl;
cout << "       \n";
//cout << "\n" << endl;

for (i=0; i!=np; i++){
cout << i+1 << ' ';
}
cout << '\n';
cout << "       \n";
cout << "[links]"<<endl;
cout << "       \n";

for (i=0; i!=(np*3); i++){
a = rand() % (np)+1;
b = rand() % (np)+1;
c = (float)(rand() % 101)/100+0.01;
A.push_back(a);
B.push_back(b);
if (a!=b){
cout << '('<< a <<','<<b<<')' << ' '<< "delay " << c << " prob 0.0"<<endl;
}
}
cout << "       \n";
cout << "[events]"<<endl;
cout << "       \n";

for (i=0; i!=(np); i++){
a = rand() % (np)+1;
b = rand() % (np)+1;
if ((rand() % 51) == 44){

cout<< 20*i <<' '<< "linkdying " <<'('<< A[a] <<','<< B[a] <<')' <<endl;
tempa=A[a];
tempb=B[a];
//tempt=20*i;
flag=true;
}
else if ((rand() % 51) == 33 && flag==true){
cout<< 20*i <<' '<< "linkcomingup " <<'('<< tempa <<','<< tempb <<')' <<endl;
flag=false;
}
else{
cout<< 20*i <<' '<< "xmit " <<'('<< a <<','<< b <<')' <<endl;
}
EndTime=20*i+50;
}
cout<< EndTime <<" end"<<endl;
//cout << b <<endl;
//cout << c <<endl;
return 0;
}
