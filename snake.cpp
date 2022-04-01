#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <random>
#include <stdio.h>

#include <unistd.h>
#include <termios.h>

#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

enum way{ LEFT, RIGHT, UP, DOWN };

mutex fruitMtx;
condition_variable condFruit;

const int WIDTH = 80;
const int HEIGHT = 30;
vector<vector<int>> vecMatrix;

int DIRECTION_ID = 0;
int funcChangeDirCounter = 0;

int SNAKE_SPEED = 160;
int GAME_MODE = 0;
bool GAME_GO_ON = true;

int snakePosControl(vector<int> pos, vector<vector<int>> poses){
    int counter = 0;
    for(auto &i: poses){
        if(i.at(0)==pos.at(0) && i.at(1)==pos.at(1)){
            counter++;
        }
    }
    return counter;
}

class SNAKE{
public:
    int length = 3;
    vector<vector<int>> snakePos;

    queue<vector<int>> movePoints;
    queue<way> moveDirection;

    vector<int> fruitPos;
    bool FRUIT_READY = false;
    bool FRUIT_EATEN = false;

    vector<way> direction;
    vector<int> directionID;

    SNAKE(){
        for(int i=0; i<length; i++){
            vector<int> pos;

            pos.push_back(WIDTH/2 - i);
            pos.push_back(HEIGHT/2);
            snakePos.push_back(pos);

            direction.push_back(RIGHT);

            directionID.push_back(0);
        }
        for (int i = 0; i < 2; i++) {
            fruitPos.push_back(0);
        }
    };

    void moveON(){
        for(int i=0; i<length; i++){
            if(direction.at(i) == RIGHT){
                snakePos.at(i).at(0)++;
            }else if(direction.at(i) == LEFT){
                snakePos.at(i).at(0)--;
            }else if(direction.at(i) == UP){
                snakePos.at(i).at(1)--;
            }else if(direction.at(i) == DOWN){
                snakePos.at(i).at(1)++;
            }
        }

    }

    void updateMatrix(){

        ///
        for(int i=0; i<HEIGHT; i++){
            for(int j=0; j<WIDTH; j++){
                if( i==0 || j==0 || i==HEIGHT-1 || j==WIDTH-1){
                    vecMatrix[i][j] = 0;
                }else{
                    vecMatrix[i][j] = 1;
                }
            }
        }
        ///

        for(auto &pos: snakePos){
            //cout << "Pos: " << pos.at(1) << ", " << pos.at(0) << "\t";
            vecMatrix[pos.at(1)][pos.at(0)] = 2;
        }


        if(FRUIT_READY){
            vecMatrix[fruitPos.at(1)][fruitPos.at(0)] = 3;
        }

    }
    int t = 1;
    void checkPos(){

        if(GAME_MODE != 0){
            vector<int> currPos = snakePos.at(0);
            vector<vector<int>> currPoses = snakePos;

            if( snakePosControl(currPos, currPoses)>1 ||
                currPos.at(1)==0 || currPos.at(0)==0 || currPos.at(1)==HEIGHT-1 || currPos.at(0)==WIDTH-1 )
            {
                t *= -1;
                GAME_GO_ON += t;
            }

        }

        for(auto &pos: snakePos){

            if(pos.at(0)>=WIDTH-1){
                pos.at(0) = 1;
            }else if(pos.at(0)<=0){
                pos.at(0) = WIDTH-2;
            }

            if(pos.at(1)>=HEIGHT-1){
                pos.at(1) = 1;
            }else if(pos.at(1)<=0){
                pos.at(1) = HEIGHT-2;
            }

        }

    }

    void plusBodyPiece(){

        vector<int> newPos;

        way currDir = direction.back();

        int x, y;

        if( currDir == UP ){
            x = snakePos.back().at(0);
            y = snakePos.back().at(1) + 1;
        }else if( currDir == DOWN ){
            x = snakePos.back().at(0);
            y = snakePos.back().at(1) - 1;
        }else if( currDir == LEFT ){
            x = snakePos.back().at(0) + 1;
            y = snakePos.back().at(1);
        }else if( currDir == RIGHT ){
            x = snakePos.back().at(0) - 1;
            y = snakePos.back().at(1);
        }

        newPos.push_back(x);
        newPos.push_back(y);

        length++;
        snakePos.push_back(newPos);
        direction.push_back(currDir);

        directionID.push_back(directionID.back());

        if(SNAKE_SPEED>35){
            SNAKE_SPEED-=5;
        }else if(SNAKE_SPEED>30){
            SNAKE_SPEED-=1;
        }
    }

    int numOfID(int id){
        int  counter = 0;
        for(auto &i: directionID){
            if(i == id){
                counter++;
            }
        }
        return counter;
    }


};

void show();
void snakeBody();

int numberGenerator(int a, int b);
char getch();

void input();
void changeDirection();
void changeDirThread(vector<int> pos, way dir, int id);
void produceFruit();
void isFruitEaten();

SNAKE Snake;

int main() {
    // Fill the matrix
    for(int i=0; i<HEIGHT; i++){
        vector<int> tmp;
        for(int j=0; j<WIDTH; j++){
            if( i==0 || j==0 || i==HEIGHT-1 || j==WIDTH-1){
                tmp.push_back(0);
            }else{
                tmp.push_back(1);
            }
        }
        vecMatrix.push_back(tmp);
    }

    // Show matrix
    thread showMx(show);
    showMx.detach();

    // Input
    thread inputThread(input);
    inputThread.detach();

    // Change Direction
    thread changeDir(changeDirection);
    changeDir.detach();

    // Produce fruit
    thread fruit(produceFruit);
    fruit.detach();

    // Is fruit eaten?
    thread isFEat(isFruitEaten);
    isFEat.detach();

    snakeBody();

    getchar();
    return 0;
}

void changeDirection(){

    while(true){
        while(Snake.moveDirection.size()>0){
            funcChangeDirCounter++;

            vector<int> tmp = Snake.movePoints.front();
            way dir = Snake.moveDirection.front();

            Snake.movePoints.pop();
            Snake.moveDirection.pop();

            thread newChange(changeDirThread, tmp, dir, DIRECTION_ID);
            newChange.detach();
            DIRECTION_ID++;

        }
    }
}
void changeDirThread(vector<int> pos, way dir, int id=-1){

    while(Snake.numOfID(id)<=Snake.length){
        for(int i=0; i<Snake.length; i++){
            //cout << "Pos: " << Snake.snakePos.at(i).at(0) << ", " << Snake.snakePos.at(i).at(1) << endl;
            if(Snake.snakePos.at(i).at(0) == pos.at(0) && Snake.snakePos.at(i).at(1) == pos.at(1) && Snake.directionID.at(i) == id){
                Snake.direction.at(i) = dir;
                Snake.directionID.at(i)++;
            }
            //cout << "Counter: " << counter << endl;
        }
        this_thread::sleep_for(20ms);
        //this_thread::sleep_for(100ms);
    }
}

void input(){

    while(1){
        this_thread::sleep_for(110ms);

        char key;
        key = getch();
        // Turn point
        //cout << "Snake.snakePos.front: " << Snake.snakePos.front().at(1) << ", " << Snake.snakePos.front().at(0) << endl;
        vector<int> tmp;
        for(int i=0; i<Snake.length; i++){
            tmp.push_back(i);
        }

        way currentDir = Snake.direction.at(0);

        if( key == 'w' || key == 'W' ){ // UP
            if(currentDir != UP && currentDir != DOWN){
                //Snake.waitForChangeDir.push(tmp);
                Snake.movePoints.push(Snake.snakePos.front());
                Snake.moveDirection.push(UP);
            }
        }else if( key == 's' || key == 'S' ){ // DOWN
            if(currentDir != DOWN && currentDir != UP){
                //Snake.waitForChangeDir.push(tmp);
                Snake.movePoints.push(Snake.snakePos.front());
                Snake.moveDirection.push(DOWN);
            }
        }else if( key == 'a' || key == 'A' ){ // LEFT
            if(currentDir != LEFT && currentDir != RIGHT){
                //Snake.waitForChangeDir.push(tmp);
                Snake.movePoints.push(Snake.snakePos.front());
                Snake.moveDirection.push(LEFT);
            }
        }else if( key == 'd' || key == 'D' ){ // RIGHT
            if(currentDir != RIGHT && currentDir != LEFT){
                //Snake.waitForChangeDir.push(tmp);
                Snake.movePoints.push(Snake.snakePos.front());
                Snake.moveDirection.push(RIGHT);
            }
        }

    }

}

void show(){
    while(1){
        cout << "Length: " << Snake.length << endl;
        cout << "Snake.snakePos.size(): " << Snake.snakePos.size() << endl;
        cout << "Snake.movePoints.size(): " << Snake.movePoints.size() << endl;
        cout << "funcChangeDirectionCounter: " << funcChangeDirCounter << endl;
        cout << "Snake Speed: " << SNAKE_SPEED << endl;
        cout << "Game State: " << GAME_GO_ON << endl;

        cout << endl << "SCORE: " << Snake.length << endl << endl;

        for(auto &vec: vecMatrix){
            for(auto &tmp: vec){
                if( tmp==0 ){
                    cout << "□ ";
                    //cout << "\x1B[34m□ \033[0m";
                }else if( tmp==2 ){
                    cout << "# ";
                    //cout << "\033[32m# \033[0m";
                    //cout << "\x1B[36m# \033[0m";
                }else if( tmp==1){
                    cout << "  ";
                }else if( tmp==3 ){
                    cout << "@ ";
                }
            }
            cout << endl;
        }

        this_thread::sleep_for(80ms);

        //system("clear");
        printf("\e[1;1H\e[2J");
    }

}

void snakeBody(){
    while(1){
        Snake.updateMatrix();
        Snake.moveON();
        Snake.checkPos();
        this_thread::sleep_for(chrono::milliseconds(SNAKE_SPEED));
    }
}

void isFruitEaten(){
    while(true){
        while(Snake.FRUIT_READY){
            vector<int> currPos = Snake.snakePos.at(0);
            if( currPos.at(0) == Snake.fruitPos.at(0) && currPos.at(1) == Snake.fruitPos.at(1) ){
                Snake.FRUIT_EATEN = true;
                Snake.FRUIT_READY = false;
                unique_lock<mutex> lckFruit(fruitMtx);
                condFruit.notify_all();

                Snake.plusBodyPiece();
            }
        }
    }
}

void produceFruit(){
    while(true){
        this_thread::sleep_for(2000ms);

        int x, y;
        bool COND = true;

        while(COND){
            x = numberGenerator(1, WIDTH-2); // [1, WIDTH-2]
            y = numberGenerator(1, HEIGHT-2); // [1, HEIGHT-2]

            int counter = 1;
            int currentLength = Snake.length;

            for(auto &pos: Snake.snakePos){
                if(pos.at(0) == x || pos.at(1) == y){
                    break;
                }else{
                    counter++;
                }
            }

            if(counter == currentLength){
                COND = false;
            }
        }

        Snake.fruitPos.at(0) = x;
        Snake.fruitPos.at(1) = y;
        Snake.FRUIT_READY = true;

        unique_lock<mutex> lckFruit(fruitMtx);
        while(!Snake.FRUIT_EATEN) condFruit.wait(lckFruit);
        Snake.FRUIT_EATEN = false;
    }
}

int numberGenerator(int a, int b){
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(a,b); // distribution in range [a, b]

    return dist6(rng);
}

char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return (buf);
}