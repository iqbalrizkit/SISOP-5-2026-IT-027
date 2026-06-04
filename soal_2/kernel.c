int cursor = 0;
char color = 0x07;

void putInMemory(int segment, int address, char character);
int getChar();

void printChar(char c);
void printString(char* str);
void newline();
void clearScreen();
void readString(char* str);

void printChar(char c) {
    putInMemory(0xB800, cursor * 2, c);
    putInMemory(0xB800, cursor * 2 + 1, color);
    cursor++;
}

void printString(char* str) {
    int i = 0;
    while (str[i] != 0) {
        printChar(str[i]);
        i++;
    }
}

void newline() {
    int temp = cursor;
    int current_line = 0;
    while (temp >= 80) {
        temp = temp - 80;
        current_line++;
    }
    cursor = (current_line + 1) * 80;
}

void clearScreen() {
    int i;
    cursor = 0;
    for (i = 0; i < 80 * 25; i++) {
        putInMemory(0xB800, i * 2, ' ');
        putInMemory(0xB800, i * 2 + 1, 0x07);
    }
}

void readString(char* str) {
    int i = 0;
    char c;
    while (1) {
        c = getChar();

        if (c == 13) {
            str[i] = 0;
            break;
        }
        else if (c == 8) {
            if (i > 0) {
                i--;
                cursor--;
                printChar(' ');
                cursor--;
            }
        }
        else if (c >= 32 && c <= 126) {
            if (i < 63) {
                str[i] = c;
                printChar(c);
                i++;
            }
        }
    }
}

int strcmp(char* a, char* b) {
    int i = 0;
    while (a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

int startsWith(char* str, char* prefix) {
    int i = 0;
    while (prefix[i] != 0) {
        if (str[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

int atoi(char* str) {
    int result = 0;
    int i = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result;
}

void intToString(int num, char* str) {
    char temp[10];
    int i = 0;
    int j = 0;

    if (num == 0) {
        str[0] = '0';
        str[1] = 0;
        return;
    }

    while (num > 0) {
        int rem = num;
        int quot = 0;
        while (rem >= 10) {
            rem = rem - 10;
            quot++;
        }
        temp[i] = rem + '0';
        i++;
        num = quot;
    }

    while (i > 0) {
        i--;
        str[j] = temp[i];
        j++;
    }
    str[j] = 0;
}

int factorial(int n) {
    int hasil = 1;
    int i;
    for (i = 1; i <= n; i++) {
        hasil = hasil * i;
    }
    return hasil;
}

void main() {
    char cmd[64];
    clearScreen();

    printString("Welcome to Shell");
    newline();
    printString("type 'help'");
    newline();
    newline();

    while (1) {
        printString("> ");
        readString(cmd);
        newline();

        if (strcmp(cmd, "check")) {
            printString("ok");
        }
        else if (startsWith(cmd, "add ")) {
            int i = 4, a, b;
            char hasil[10];
            
            while (cmd[i] == ' ') i++;
            a = atoi(cmd + i);
            
            while (cmd[i] >= '0' && cmd[i] <= '9') i++;
            while (cmd[i] == ' ') i++;
            b = atoi(cmd + i);

            intToString(a + b, hasil);
            printString(hasil);
        }
        else if (startsWith(cmd, "sub ")) {
            int i = 4, a, b, res;
            char hasil[10];
            
            while (cmd[i] == ' ') i++;
            a = atoi(cmd + i);
            
            while (cmd[i] >= '0' && cmd[i] <= '9') i++;
            while (cmd[i] == ' ') i++;
            b = atoi(cmd + i);

            res = a - b;
            if (res < 0) {
                printChar('-');
                res = -res;
            }
            intToString(res, hasil);
            printString(hasil);
        }
        else if (startsWith(cmd, "fac ")) {
            int n = atoi(cmd + 4);
            char hasil[10];

            if (n > 8) {
                printString("know your limit little bro.");
            } else {
                intToString(factorial(n), hasil);
                printString(hasil);
            }
        }
        else if (startsWith(cmd, "season ")) {
            char* name = cmd + 7;
            if (strcmp(name, "winter")) {
                color = 0x09;
                printString("winter mode");
            } else if (strcmp(name, "spring")) {
                color = 0x0D;
                printString("spring mode");
            } else if (strcmp(name, "summer")) {
                color = 0x0A;
                printString("summer mode");
            } else if (strcmp(name, "fall")) {
                color = 0x0E;
                printString("fall mode");
            } else if (strcmp(name, "radiant")) {
                color = 0x0C;
                printString("radiant mode");
            } else {
                printString("season not found");
            }
        }
        else if (startsWith(cmd, "triangle ")) {
            int n = atoi(cmd + 9);
            int i, j;
            for (i = 1; i <= n; i++) {
                for (j = 0; j < i; j++) {
                    printChar('x');
                }
                if (i < n) newline();
            }
        }
        else if (strcmp(cmd, "clear")) {
            clearScreen();
            continue;
        }
        else if (strcmp(cmd, "about")) {
            printString("OS Shell - Final Challenge Modul 5");
        }
        else if (strcmp(cmd, "help")) {
            printString("check add sub fac season triangle clear about help");
        }
        else {
            printString("unknown command");
        }
        newline();
    }
}
