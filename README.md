# Soal 2 - Season

# Gambaran Umum Alur Program

Secara besar, alur sistem berjalan seperti ini:

```text
BIOS
  ↓
bootloader.asm
  ↓
kernel.asm
  ↓
kernel.c
  ↓
shell command sederhana
```

`bootloader.asm` bertugas membaca kernel dari floppy image dan memindahkannya ke alamat memori `0x1000:0000`. Setelah kernel berhasil dibaca, bootloader melakukan far jump ke alamat tersebut agar eksekusi berpindah ke kernel.

`kernel.asm` menjadi jembatan antara dunia assembly dan C. File ini menyediakan entry point `_start`, fungsi `_putInMemory` untuk menulis langsung ke memori video, dan fungsi `_getChar` untuk membaca input keyboard melalui BIOS interrupt.

`kernel.c` berisi logika utama shell. Di dalam file ini dibuat fungsi-fungsi terminal sederhana seperti `printChar`, `printString`, `readString`, `clearScreen`, parsing command, operasi matematika, pengubahan warna, dan command tambahan lain.

---

## Struktur File yang Digunakan

Soal menyediakan 7 file template:

```text
bochsrc.txt
bootloader.asm
build.sh
kernel.asm
kernel.c
Makefile
README.md
```

Peran tiap file adalah sebagai berikut.

`bochsrc.txt` digunakan sebagai konfigurasi Bochs. File ini menentukan jumlah memori, lokasi BIOS, lokasi VGA BIOS, floppy image yang dipakai, jenis boot device, log file, dan display library. Jika menggunakan Bochs di Windows, path ROM dan `floppy.img` harus disesuaikan dengan lokasi file di Windows.

`bootloader.asm` adalah program 512 byte pertama yang dijalankan BIOS ketika boot dari floppy. File ini harus diakhiri dengan signature `0xAA55`, karena BIOS hanya menganggap sektor pertama sebagai bootable jika dua byte terakhirnya adalah `55 AA`.

`build.sh` berisi alternatif script build. Pada beberapa environment, build dapat dilakukan lewat `make build`, tetapi script ini tetap berguna apabila ingin membangun image secara manual atau melalui Docker.

`kernel.asm` berisi fungsi low-level yang tidak bisa langsung dibuat dengan C biasa. Fungsi pentingnya adalah `_getChar` untuk input keyboard dan `_putInMemory` untuk menulis ke memori video.

`kernel.c` adalah inti fitur soal. Semua command seperti `check`, `add`, `sub`, `fac`, `season`, `triangle`, `clear`, `help`, dan `about` diproses di file ini.

`Makefile` mengatur proses build. File ini membuat `floppy.img`, meng-assemble bootloader, meng-assemble `kernel.asm`, meng-compile `kernel.c`, melakukan linking, lalu menulis bootloader dan kernel ke floppy image.

`README.md` digunakan untuk menjelaskan cara build, cara menjalankan, dan daftar fitur sistem.

---
## 1 - Mengisi Fungsi `_getChar`

Poin pertama soal meminta untuk mengisi fungsi `_getChar` pada `kernel.asm`. Fungsi ini digunakan agar sistem dapat membaca input keyboard dari pengguna.
Karena sistem berjalan dalam mode real 16-bit dan tidak memakai library input modern, pembacaan keyboard dilakukan menggunakan BIOS interrupt `int 0x16`.

### Implementasi

Fungsi `_getChar` dibuat seperti berikut:

```asm
_getChar:
    mov ah, 0x00
    int 0x16
    xor ah, ah
    ret
```

### Penjelasan Teknis

Instruksi:

```asm
mov ah, 0x00
```

memilih layanan BIOS keyboard dengan fungsi `00h`. Fungsi ini berarti menunggu tombol keyboard ditekan, lalu mengambil karakter tersebut.

Instruksi:

```asm
int 0x16
```

memanggil BIOS keyboard interrupt. Setelah interrupt selesai, karakter ASCII dari tombol yang ditekan akan berada di register `AL`, sedangkan scan code keyboard berada di `AH`.

Karena fungsi ini dipanggil dari C sebagai:

```c
int getChar();
```

maka nilai return dibaca dari register `AX`. Agar nilai return bersih dan hanya berisi ASCII, register `AH` dikosongkan dengan:

```asm
xor ah, ah
```

Jika `AH` tidak dikosongkan, hasil input dapat tercampur dengan scan code, sehingga pembacaan karakter di `readString()` bisa tidak stabil.

### Hubungan dengan `kernel.c`

Fungsi ini dipakai di `readString()`:

```c
c = getChar();
```

Setiap kali user mengetik satu karakter, `getChar()` mengambil karakter itu dari keyboard, lalu `readString()` menyimpannya ke array command.

## 2 -  Membuat Instruksi `check`

Soal lalu meminta untuk membuat command `check` yanmg digunakan sebagai fitur uji awal untuk memastikan shell berjalan. Jika user mengetik:

```text
> check
```

maka sistem harus menampilkan:

```text
ok
```

### Fungsi Dasar yang Dibutuhkan

Sebelum `check` bisa dibuat, beberapa fungsi terminal dasar harus ada:

```c
clearScreen();
newline();
printChar();
printString();
readString();
strcmp();
```

Fungsi `clearScreen()` membersihkan layar dengan menulis karakter spasi ke seluruh area VGA text memory. Fungsi ini menulis ke segment `0xB800`, yaitu alamat standar video memory untuk mode teks warna.

Fungsi `printChar()` menulis satu karakter ke posisi cursor saat ini. Setiap karakter di VGA text mode memakai 2 byte: byte pertama untuk karakter, byte kedua untuk atribut warna.

Fungsi `printString()` memanggil `printChar()` berulang kali sampai menemukan karakter null `0`.

Fungsi `readString()` membaca input keyboard satu per satu menggunakan `getChar()`. Jika tombol Enter ditekan, string diakhiri dengan `0`. Jika Backspace ditekan, karakter sebelumnya dihapus dari layar dan index input dikurangi.

Fungsi `strcmp()` membandingkan dua string secara manual. Fungsi ini mengembalikan `1` jika dua string sama, dan `0` jika berbeda.

### Implementasi Command Handler

Di dalam loop utama `main()`, command dibaca ke array:

```c
char cmd[64];
readString(cmd);
```

Kemudian command dibandingkan:

```c
if (strcmp(cmd, "check")) {
    printString("ok");
}
```

### Alur Eksekusi

Saat user mengetik `check`, isi array `cmd` menjadi:

```text
c h e c k \0
```

Fungsi `strcmp(cmd, "check")` memeriksa karakter satu per satu. Karena semua karakter sama dan keduanya berakhir di null terminator, fungsi mengembalikan `1`. Setelah itu sistem mencetak `ok`.

## 3 - Membuat Fitur `add`

Soal lalu meminta untuk membuat command `add` yang digunakan untuk menjumlahkan dua bilangan. Contoh:

```text
> add 5 3
8
```

Karena tidak memakai `stdlib.h`, sistem tidak bisa memakai `atoi()`. Oleh karena itu, parsing angka harus dibuat sendiri.

### Fungsi yang Dibutuhkan

Fitur `add` membutuhkan tiga fungsi tambahan:

```c
startsWith();
readNumberAt();
nextNumberIndex();
```

Fungsi `startsWith()` memeriksa apakah command diawali prefix tertentu. Untuk command `add 5 3`, prefix yang dicek adalah `"add "`.

Fungsi `readNumberAt()` membaca angka mulai dari index tertentu dalam string. Misalnya pada string:

```text
add 5 3
```

index karakter adalah:

```text
a d d   5   3
0 1 2 3 4 5 6
```

Angka pertama dimulai dari index `4`.

Fungsi `nextNumberIndex()` mencari posisi awal angka berikutnya. Setelah membaca angka pertama, fungsi ini melewati digit dan spasi agar index berpindah ke angka kedua.

### Implementasi Handler

```c
} else if (startsWith(cmd, "add ")) {
    index = 4;

    a = readNumberAt(cmd, index);
    index = nextNumberIndex(cmd, index);
    b = readNumberAt(cmd, index);

    result = a + b;

    printNumber(result);
}
```

### Penjelasan Teknis

`index = 4` digunakan karena command `"add "` memiliki panjang 4 karakter. Setelah itu, `readNumberAt()` membaca bilangan pertama. Kemudian `nextNumberIndex()` mencari posisi angka kedua. Hasil penjumlahan disimpan dalam `result`, lalu dicetak dengan `printNumber()`.

Fungsi `printNumber()` juga dibuat manual. Fungsi ini tidak memakai pembagian `/` atau modulo `%`, karena template soal memberi batasan untuk menghindari operasi tersebut. Sebagai gantinya, fungsi mencetak angka dengan mengurangi nilai berdasarkan divisor `10000`, `1000`, `100`, `10`, dan `1`.

## 4 - Membuat Fitur `sub`

Soal lalu meminta untuk membuat command `sub` yang digunakan untuk mengurangi dua bilangan. Contoh:

```text
> sub 10 2
8
```

Fitur ini memakai sistem parsing angka yang sama dengan `add`, tetapi operasi akhirnya adalah pengurangan.

### Implementasi Handler

```c
} else if (startsWith(cmd, "sub ")) {
    index = 4;

    a = readNumberAt(cmd, index);
    index = nextNumberIndex(cmd, index);
    b = readNumberAt(cmd, index);

    result = a - b;

    printNumber(result);
}
```

### Penjelasan Teknis

Command `sub 10 2` diproses sebagai string. Prefix `"sub "` diperiksa dengan `startsWith()`. Angka pertama dibaca sebagai `10`, angka kedua dibaca sebagai `2`, lalu hasilnya dihitung dengan:

```c
result = a - b;
```

Fungsi `printNumber()` juga mendukung angka negatif. Jika hasil pengurangan negatif, misalnya:

```text
> sub 3 7
-4
```

maka fungsi mencetak karakter `'-'` terlebih dahulu, lalu mengubah nilai negatif menjadi positif dengan:

```c
number = 0 - number;
```

Setelah itu digit angka dicetak seperti biasa.

## 5 - Membuat Fitur `fac`

Soal lalu meminta untuk membuat command `fac` yang digunakan untuk menghitung faktorial dari sebuah bilangan. Contoh:

```text
> fac 6
720
```

Soal memberi catatan penting bahwa sistem yang dibuat adalah sistem 16-bit, sehingga ada batas integer yang harus diperhatikan. Jika nilai terlalu besar, sistem harus mencetak:

```text
know your limit little bro.
```

### Batas 16-bit

Dalam sistem 16-bit signed integer, nilai maksimum yang aman adalah sekitar `32767`. Nilai faktorial naik sangat cepat:

```text
7! = 5040
8! = 40320
```

Karena `8!` sudah melewati `32767`, maka batas aman yang dipakai adalah `7`.

Dengan demikian:

```text
fac 6   -> 720
fac 7   -> 5040
fac 8   -> know your limit little bro.
fac 120 -> know your limit little bro.
```

### Implementasi Fungsi Faktorial

```c
int factorial(int number) {
    int i;
    int result;

    i = 1;
    result = 1;

    while (i <= number) {
        result = result * i;
        i++;
    }

    return result;
}
```

### Implementasi Handler

```c
} else if (startsWith(cmd, "fac ")) {
    index = 4;

    a = readNumberAt(cmd, index);

    if (a > 7) {
        printString("know your limit little bro.");
    } else {
        result = factorial(a);
        printNumber(result);
    }
}
```

### Penjelasan Teknis

Angka setelah command `fac` dibaca mulai dari index `4`, karena `"fac "` memiliki panjang 4 karakter. Jika angka lebih besar dari 7, sistem tidak menghitung faktorialnya. Ini dilakukan untuk mencegah overflow integer.

Jika angka masih aman, fungsi `factorial()` menghitung hasil dengan loop dari `1` sampai `number`. Hasil akhir dicetak menggunakan `printNumber()`.

## 6 - Membuat Fitur Season

Soal lalu meminta untuk membuat command `season` yang digunakan untuk mengubah warna teks pada sistem. Command yang harus didukung adalah:

```text
season winter
season spring
season summer
season fall
season radiant
```

Setiap season mengubah nilai variabel global `color`. Nilai `color` kemudian dipakai oleh `printChar()` saat menulis karakter ke VGA memory.

### Mapping Warna

Mapping warna yang digunakan:

```text
winter  -> 0x09 -> biru terang
spring  -> 0x0A -> hijau terang
summer  -> 0x0E -> kuning
fall    -> 0x06 -> coklat/oranye
radiant -> 0x0D -> pink/ungu
```

### Implementasi Handler

```c
} else if (startsWith(cmd, "season ")) {
    index = 7;

    if (strcmpAt(cmd, index, "winter")) {
        color = 0x09;
        printString("winter season applied");
    } else if (strcmpAt(cmd, index, "spring")) {
        color = 0x0A;
        printString("spring season applied");
    } else if (strcmpAt(cmd, index, "summer")) {
        color = 0x0E;
        printString("summer season applied");
    } else if (strcmpAt(cmd, index, "fall")) {
        color = 0x06;
        printString("fall season applied");
    } else if (strcmpAt(cmd, index, "radiant")) {
        color = 0x0D;
        printString("radiant season applied");
    } else {
        printString("unknown season");
    }
}
```

### Penjelasan Teknis

Command `"season "` memiliki panjang 7 karakter, sehingga nama season mulai dibaca dari index `7`.

Fungsi `strcmpAt()` membandingkan substring pada posisi tertentu dengan kata target. Contohnya, pada command:

```text
season winter
```

fungsi ini membandingkan isi command mulai index `7` dengan string `"winter"`.

Setelah warna diubah, semua output berikutnya akan memakai warna baru karena fungsi `printChar()` selalu menulis atribut warna dari variabel global `color`.

## 7 - Membuat Fitur `triangle`

Soal kemudian meminta untuk membuat command `triangle` yang digunakan untuk mencetak segitiga karakter `x`. Contoh:

```text
> triangle 5
x
xx
xxx
xxxx
xxxxx
```

Fitur ini membutuhkan parsing angka seperti `add`, `sub`, dan `fac`.

### Implementasi Fungsi

```c
void printTriangle(int size) {
    int row;
    int col;

    row = 1;

    while (row <= size) {
        col = 1;

        while (col <= row) {
            printChar('x');
            col++;
        }

        newline();
        row++;
    }
}
```

### Implementasi Handler

```c
} else if (startsWith(cmd, "triangle ")) {
    index = 9;

    a = readNumberAt(cmd, index);

    if (a <= 0) {
        printString("triangle size must be positive");
    } else {
        printTriangle(a);
        needNewline = 0;
    }
}
```

### Penjelasan Teknis

Command `"triangle "` memiliki panjang 9 karakter, sehingga angka ukuran segitiga mulai dibaca dari index `9`.

Fungsi `printTriangle()` menggunakan dua loop. Loop luar menentukan baris, sedangkan loop dalam mencetak jumlah `x` sesuai nomor baris.

Untuk `triangle 5`, prosesnya adalah:

```text
baris 1 -> 1 x
baris 2 -> 2 x
baris 3 -> 3 x
baris 4 -> 4 x
baris 5 -> 5 x
```

Variabel `needNewline` digunakan agar setelah `printTriangle()` tidak ditambahkan newline ekstra yang membuat tampilan prompt terlalu renggang.

## 8 - Membuat Fitur `clear` dan `help`

Terakhir, soal meminta untuk membuat command `clear` dan `help`.  Command `clear` digunakan untuk membersihkan layar dari histori command. Command ini memanggil `clearScreen()` dan mengembalikan cursor ke posisi awal.

Command `help` digunakan untuk menampilkan daftar command yang tersedia. Output yang diminta soal adalah:

```text
check add sub fac season triangle clear about
```

Walaupun command `help` tidak tercantum dalam output daftar tersebut, fitur `help` tetap harus tersedia karena diminta oleh soal.

### Implementasi `clear`

```c
} else if (strcmp(cmd, "clear")) {
    clearScreen();
    needNewline = 0;
}
```

Fungsi `clearScreen()` mengulang seluruh 80 × 25 posisi layar. Setiap posisi diisi dengan karakter spasi dan atribut warna saat ini.

```c
for (i = 0; i < 80 * 25; i++) {
    putInMemory(0xB800, i * 2, ' ');
    putInMemory(0xB800, i * 2 + 1, color);
}
```

Setelah itu cursor dikembalikan ke awal:

```c
cursor = 0;
```

Karena layar sudah dibersihkan, `needNewline` dibuat `0` agar sistem tidak menambahkan baris kosong setelah `clear`.

### Implementasi `help`

```c
void printHelp() {
    printString("check add sub fac season triangle clear about");
}
```

Handler-nya:

```c
} else if (strcmp(cmd, "help")) {
    printHelp();
}
```

### Command `about`

Di dalam daftar help, soal juga mencantumkan `about`. Karena itu, command `about` dibuat agar ketika user mengetiknya, sistem menampilkan informasi singkat.

```c
void printAbout() {
    printString("Final Shift OS");
    newline();
    printString("Hadiah terakhir sang asisten.");
}
```

Handler-nya:

```c
} else if (strcmp(cmd, "about")) {
    printAbout();
}
```
