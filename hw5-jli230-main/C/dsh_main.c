#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <ncurses.h>
#include <dirent.h>
#include <errno.h>
#include"dsh.h"

#define WIN_HEIGHT 40
#define WIN_WIDTH 65
#define WIN1_START_X 10
#define WIN2_START_X (WIN1_START_X + WIN_WIDTH + 20)
#define WIN_START_Y 10

void printStat(WINDOW* output, char* file){
    // mvwprintw(output, order+1, 5, "Command is: %s\n", file);
    //mvwprintw(output);
    werase(output);

    int linnum = 2;
    FILE* fd;
    char result[100] = {0};
    //wprintw(output, "%s\n", file);
    fd = fopen(file, "r");
    while(fgets(result, sizeof(result), fd)) {
        mvwprintw(output, linnum++, 5, "%s", result);
    }
    fclose(fd);
    box(output, 0,0);
    wrefresh(output);
}

void printHistory(WINDOW* output, int number, char** ptr){
    // mvwprintw(output, order+1, 5, "Command is: %s\n", file);
    //mvwprintw(output);
    werase(output);
    char temp[100];
    for (int i = 0; i <= number+1; i++){
        strcpy(temp, ptr[i]);
        temp[strlen(ptr[i])] = '\0';
        mvwprintw(output, i+2, 5, "%s", temp);
    }

    box(output, 0,0);
    wrefresh(output);
}

int main(int argc, char** argv) {
    size_t size=0;
    int inputChar = 0;
    int input_y  = 1;
	int input_x  = 1;
	int prevChar = -1;
    int order = 0;
    int latest = 0;
    
    char** ptr = (char**)malloc(100*sizeof(char*));
    for (size_t i =0; i < 100; i++) {
        ptr[i] = (char*)malloc(1024*sizeof(char));
        memset(ptr[i], '\0', sizeof(ptr[i]));
    }
    dsh_init();
    printf("dsh> ");

    char folder[100];
    snprintf(folder,100,"%s/.dsh",getenv("HOME"));
    int counter = 0;
    while(1) {
        snprintf(folder,100,"%s/.dsh/%d",getenv("HOME"), counter);
        DIR* dir = opendir(folder);
        if (dir) {
            counter++;
            closedir(dir);
        } else {
            counter--;
            snprintf(folder,100,"%s/.dsh/%d",getenv("HOME"), counter);
            break;
        }
    }
    printf("Folder is %i\n", counter);
    char filedest[200];
    char filedesterr[200];
    int filecount = 0;
    snprintf(filedest,200,"%s/%d.stdout",folder, filecount);
    snprintf(filedesterr,200,"%s/%d.stderr",folder, filecount);


    initscr();
    cbreak();
    noecho();

    mvwprintw(stdscr, WIN_START_Y+3, WIN1_START_X, "%s", "Typing Window");
    WINDOW *input = newwin(WIN_HEIGHT/2, WIN_WIDTH, WIN_START_Y+5, WIN1_START_X);
    mvwprintw(stdscr, WIN_START_Y-10, WIN1_START_X, "%s", "Output Window");
    WINDOW *output = newwin(WIN_HEIGHT-30, WIN_WIDTH, WIN_START_Y-8, WIN1_START_X);
    mvwprintw(stdscr, WIN_START_Y-10, WIN2_START_X, "%s", "History Window");
    WINDOW *history = newwin(WIN_HEIGHT, WIN_WIDTH, WIN_START_Y-8, WIN2_START_X);
    refresh();
    box(input,0,0);
    wrefresh(input);
    box(output, 0,0);
    wrefresh(output);
    box(history, 0,0);
    wrefresh(history);
    keypad(input, TRUE);
    while(1) {
        int position = 0;
        int counter = order;
        int length = 0;
        int endposition = 0;
        char line[1024] = {0};
        int accessedhistory=0;
        werase(input);
        box(input, 0,0);
        wrefresh(input);
        //wrefresh(output);
        if (input_x == WIN_WIDTH - 1){
			input_x = 1;
			input_y++;
		}
        wmove(input, input_y, input_x);
        wrefresh(input);
        
        while(1){
            //mvwprintw(input, input_y+1, 1, "Command:%s Position: %i End of string at: %i", line, position, endposition);
            //mvwprintw(input, input_y+2, 1, "Highlighted char: %c", line[position]);
            /* Go to next line if width is exhausted */
            if (input_x == WIN_WIDTH - 1){
                input_x = 1;
                input_y++;
            }

            wmove(input, input_y, input_x);
            wrefresh(input);
            /* TODO 2a: Use wgetch to take single character input */
            inputChar = wgetch(input);

            // handle enter
            if(inputChar == '\n') {
                line[endposition++] = '\n';
                input_x=1;
                break;
            }
            else if(inputChar == KEY_BACKSPACE) {
                if (input_x == 1) {
                    continue;
                }
                input_x--;
                for (int i = position-1; i < endposition; i++) {
                    line[i] = line[i+1];
                }
                position--;
                endposition--;
                line[endposition] = '\0';
                // werase(input);
			    wmove(input, input_y, input_x);
			    wdelch(input);
                box(input, 0,0);
			    // winsch(input, ' ');
			    wrefresh(input);
			    continue;
		    } else if (inputChar == KEY_UP) {
                if (counter <= 1) {
                    if (order != 1) {
                        continue;
                    } 
                } 
                if (accessedhistory == 0){
                    accessedhistory = 1;
                } else {
                    mvwprintw(history, 2+counter, 4, " ", ptr[counter]);
                    if (order != 1) {
                        counter--;
                    }
                }
                int endline;
                for (int i = 0; i < 1024; i++) {
                    line[i] = '\0';
                }
                length = 0;
                for (int i=0; ptr[counter][i] != '\n'; i++) {
                    line[i] = ptr[counter][i];
                    length++;
                    endline = i+1;
                }
                endposition = length;
                line[endline] = '\0';
                werase(input);
                mvwprintw(input, input_y, 1, "%s", ptr[counter]);
                //mvwprintw(input, input_y+1, 1, "Counter position: %i Order num: %i", counter,order);
                mvwprintw(history, 2+counter, 4, ">", ptr[counter]);
                box(input, 0,0);
                wrefresh(input);
                wrefresh(history);
                // strcpy(line, ptr[counter]);
                continue;
            } else if (inputChar == KEY_DOWN){
                if (counter == order) {
                    continue;
                }
                if (counter != 0) {
                    mvwprintw(history, 2+counter, 4, " ", ptr[counter-1]);
                }
                counter++;
                int endline;
                for (int i = 0; i < 1024; i++) {
                    line[i] = '\0';
                }
                length = 0;
                for (int i=0; ptr[counter][i] != '\n'; i++) {
                    line[i] = ptr[counter][i];
                    length++;
                    endline = i+1;
                }
                endposition = length;
                line[endline] = '\0';
                mvwprintw(input, input_y, 1, "%s", ptr[counter]);
                mvwprintw(history, 2+counter, 4, ">", ptr[counter]);
                box(input, 0,0);
                wrefresh(input);
                wrefresh(history);
                // strcpy(line, ptr[counter]);
                continue;
            } else if (inputChar == KEY_LEFT) {
                if (input_x == 1) {
                    continue;
                }
                input_x--;
                position--;
			    wmove(input, input_y, input_x);
                continue;
            } else if (inputChar == KEY_RIGHT) {
                if (input_x == endposition+1) {
                    continue;
                }
                input_x++;
                position++;
			    wmove(input, input_y, input_x);
                continue;
            }
            prevChar = inputChar;
            line[position] = inputChar;
            position++;
            if (position>endposition) {
                endposition++;
            }
            length++;
            mvwprintw(input,input_y,input_x,"%c",inputChar);
            input_x++;
        }
        //memset(ptr[order], '\0', sizeof(ptr[order]));
        latest++;
        mvwprintw(output, 8, 5, "%s", line);
        wrefresh(output);
        strcpy(ptr[latest], line);

        dsh_run(line);
        char command[400] = {0};
        char result[200];
        //wprintw(output, "%s", ptr[order]);
        printStat(output, filedest);
        printHistory(history, order, ptr);
        //printStat(output, filedest, order);
        // snprintf(command, 400, "screen -X clear; screen -X exec cat %s;", filedest);
        // //printf("%s\n", command);
        // //dsh_run("screen");
        // dsh_run(command);
        // ~/.dsh/0/1.stdout
        // int linnum = 4;
        // FILE* fd;
        // wprintw(output, "%s\n", filedest);
        // fd = fopen(filedest, "r");
        // while(fgets(result, sizeof(result), fd)) {
        //     mvwprintw(output, linnum++, 5, "%s", result);
        // }
        wrefresh(output);
        // print each line in the output window
        

        //printf("\ndsh>");
        filecount += 1;
        order++;
        snprintf(filedest,200,"%s/%d.stdout",folder, filecount);
        snprintf(filedesterr,200,"%s/%d.stderr",folder, filecount);
        // for (int i = 0; i < 5; i++) {
        //     printf("Command printed: %s\n", ptr[i]);
        // }
   }
   endwin();
   return 0;
}