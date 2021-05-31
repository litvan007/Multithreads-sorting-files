#include <iostream>
#include <fstream>
#include <vector>
#include <random>

using namespace std;

//64000000*32767

int main(){
    ofstream out;
    out.open("./test_file.bin", ios::binary);
    if (!out.is_open()){
        cout << "File Error!" << endl;
    }
    else {
        long long int n = 64*32767; 
        int32_t num;
        for (long long int i = 0; i < n; ++i){
            num = rand()%100;
            out.write((char*)&num, sizeof(int32_t));
        }
        out.close();
    }
}
