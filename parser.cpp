#include <iostream>
#include <fstream> 
#include <string>
using namespace std;

int main() {
    string myText;

    ifstream MyReadFile("Untitled.obj");
    ofstream ParsedObject("Parsed.txt");

    while (getline (MyReadFile, myText)) {
        // find if it starts with f, get the vertices if it starts with v and vt (build the line),
        if (myText[0] == 'f')
            // rotate counter in perspective of normal using proj
            // instance the line of code in terms of ccw
            ParsedObject << myText[0] << '\n';
    }

    ParsedObject.close();
    MyReadFile.close(); 

    return 0;
}
