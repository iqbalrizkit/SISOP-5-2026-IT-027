# SISOP-5-2026-IT-027

## Member:
| Nama | NRP | Kelas |
| :---: | :---: | :---: |
| Iqbal Rizki Muhammad Fadhli | 5027251027 | Sistem Operasi B | 

## Reporting:
### Soal 2 - Season

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
# Penjelasan Langkah-langkah Penyelesaian:

## 1 - Mengisi Fungsi `_getChar`

Poin pertama soal meminta untuk mengisi fungsi `_getChar` pada `kernel.asm`. Fungsi ini digunakan agar sistem dapat membaca input keyboard dari pengguna.
Karena sistem berjalan dalam mode real 16-bit dan tidak memakai library input modern, pembacaan keyboard dilakukan menggunakan BIOS interrupt `int 0x16`.

### Implementasi

Fungsi `_getChar` dibuat seperti berikut:

```asm
_getChar:
    mov ah, 0x00        ; BIOS interrupt untuk membaca keyboard
    int 0x16            ; Panggil interupsi BIOS, hasil karakter masuk ke AL
    mov ah, 0x00        ; Bersihkan AH agar nilai kembalian murni AL
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
mov ah, 0x00
```

Jika `AH` tidak dikosongkan, hasil input dapat tercampur dengan scan code, sehingga pembacaan karakter di `readString()` bisa tidak stabil.

### Hubungan dengan `kernel.c`

Fungsi ini dipakai di `readString()`:

```c
c = getChar();
```

Setiap kali user mengetik satu karakter, `getChar()` mengambil karakter itu dari keyboard, lalu `readString()` menyimpannya ke array command.

## 2 - Membuat Instruksi `check`

Soal lalu meminta untuk membuat command `check` yang digunakan sebagai fitur uji awal untuk memastikan shell berjalan. Jika user mengetik:

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

Fungsi `clearScreen()` membersihkan layar dengan menulis karakter spasi ke seluruh area VGA text memory. Fungsi ini menulis ke segment `0xB800`, yaitu alamat standar video memory untuk mode teks warna. Perlu diperhatikan, atribut warna di `clearScreen()` di-hardcode ke `0x07` (putih di atas hitam) agar tampilan selalu bersih secara konsisten:

```c
void clearScreen() {
    int i;
    cursor = 0;
    for (i = 0; i < 80 * 25; i++) {
        putInMemory(0xB800, i * 2, ' ');
        putInMemory(0xB800, i * 2 + 1, 0x07);
    }
}
```

Fungsi `printChar()` menulis satu karakter ke posisi cursor saat ini. Setiap karakter di VGA text mode memakai 2 byte: byte pertama untuk karakter, byte kedua untuk atribut warna dari variabel global `color`.

Fungsi `printString()` memanggil `printChar()` berulang kali sampai menemukan karakter null `0`.

Fungsi `readString()` membaca input keyboard satu per satu menggunakan `getChar()`. Jika tombol Enter ditekan (kode ASCII `13`), string diakhiri dengan `0`. Jika Backspace ditekan (kode ASCII `8`), karakter sebelumnya dihapus dari layar dan index input dikurangi. Hanya karakter printable (ASCII `32`–`126`) yang diterima, dengan panjang maksimal 63 karakter.

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

### Fungsi yang Dibutuhkan

Fitur `add` membutuhkan dua fungsi tambahan:

```c
atoi();
intToString();
```

Fungsi `atoi()` membaca angka dari sebuah pointer string. Fungsi ini membaca digit satu per satu selama karakter masih berada di rentang `'0'`–`'9'`, lalu mengembalikan nilai integer-nya.

Fungsi `intToString()` mengubah integer menjadi string karakter. Fungsi ini bekerja dengan mengekstrak digit dari belakang menggunakan pengurangan berulang (karena tidak memakai operator modulo `%`), menyimpannya ke buffer sementara, lalu membalik urutannya.

### Implementasi Handler

```c
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
```

### Penjelasan Teknis

`i = 4` digunakan karena command `"add "` memiliki panjang 4 karakter. Setelah itu, spasi ekstra dilewati, lalu `atoi(cmd + i)` membaca bilangan pertama dengan pointer aritmetika. Kemudian index digeser melewati digit angka pertama dan spasi berikutnya, lalu `atoi(cmd + i)` membaca bilangan kedua. Hasil penjumlahan dikonversi ke string dengan `intToString()` dan dicetak dengan `printString()`.

## 4 - Membuat Fitur `sub`

Soal lalu meminta untuk membuat command `sub` yang digunakan untuk mengurangi dua bilangan. Contoh:

```text
> sub 10 2
8
```

Fitur ini memakai sistem parsing angka yang sama dengan `add`, tetapi operasi akhirnya adalah pengurangan.

### Implementasi Handler

```c
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
```

### Penjelasan Teknis

Command `sub 10 2` diproses sebagai string. Prefix `"sub "` diperiksa dengan `startsWith()`. Angka pertama dibaca sebagai `10`, angka kedua dibaca sebagai `2`, lalu hasilnya dihitung dengan:

```c
res = a - b;
```

Fungsi ini juga mendukung angka negatif. Jika hasil pengurangan negatif, misalnya:

```text
> sub 3 7
-4
```

maka karakter `'-'` dicetak terlebih dahulu dengan `printChar('-')`, lalu nilai dibalik menjadi positif dengan `res = -res`, setelah itu digit angka dicetak seperti biasa menggunakan `intToString()`.

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
8! = 40320
9! = 362880
```

Karena `9!` sudah melewati `32767`, maka batas yang diimplementasikan adalah `n > 8`:

```text
fac 8   -> 40320
fac 9   -> know your limit little bro.
fac 120 -> know your limit little bro.
```

### Implementasi Fungsi Faktorial

```c
int factorial(int n) {
    int hasil = 1;
    int i;
    for (i = 1; i <= n; i++) {
        hasil = hasil * i;
    }
    return hasil;
}
```

### Implementasi Handler

```c
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
```

### Penjelasan Teknis

Angka setelah command `fac` dibaca dengan `atoi(cmd + 4)`, karena `"fac "` memiliki panjang 4 karakter. Jika angka lebih besar dari 8, sistem tidak menghitung faktorialnya untuk mencegah overflow integer. Jika masih aman, fungsi `factorial()` menghitung hasil dengan loop dari `1` sampai `n`. Hasil akhir dikonversi ke string dengan `intToString()` dan dicetak menggunakan `printString()`.

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
spring  -> 0x0D -> pink/magenta
summer  -> 0x0A -> hijau terang
fall    -> 0x0E -> kuning
radiant -> 0x0C -> merah terang
```

### Implementasi Handler

```c
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
```

### Penjelasan Teknis

Command `"season "` memiliki panjang 7 karakter, sehingga nama season diambil langsung dengan pointer aritmetika `char* name = cmd + 7`.

Fungsi `strcmp()` membandingkan pointer `name` dengan string target. Contohnya, pada command:

```text
season winter
```

`name` menunjuk ke substring `"winter"` di dalam array `cmd`, lalu dibandingkan dengan literal `"winter"`.

Setelah warna diubah, semua output berikutnya akan memakai warna baru karena fungsi `printChar()` selalu menulis atribut warna dari variabel global `color`. Jika nama season tidak dikenali, sistem mencetak `"season not found"`.

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

### Implementasi Handler

```c
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
```

### Penjelasan Teknis

Command `"triangle "` memiliki panjang 9 karakter, sehingga angka ukuran segitiga dibaca dengan `atoi(cmd + 9)`.

Implementasi menggunakan dua loop. Loop luar menentukan baris dari `1` sampai `n`, sedangkan loop dalam mencetak jumlah `x` sesuai nomor baris. Fungsi `newline()` hanya dipanggil di antara baris (kondisi `i < n`), sehingga tidak ada newline ekstra setelah baris terakhir dan tampilan prompt tidak terlalu renggang.

Untuk `triangle 5`, prosesnya adalah:

```text
baris 1 -> 1 x
baris 2 -> 2 x
baris 3 -> 3 x
baris 4 -> 4 x
baris 5 -> 5 x
```

## 8 - Membuat Fitur `clear` dan `help`

Terakhir, soal meminta untuk membuat command `clear` dan `help`. Command `clear` digunakan untuk membersihkan layar dari histori command. Command ini memanggil `clearScreen()` dan mengembalikan cursor ke posisi awal, lalu langsung melanjutkan loop dengan `continue` agar tidak mencetak newline tambahan setelah layar dibersihkan.

Command `help` digunakan untuk menampilkan daftar command yang tersedia. Output yang ditampilkan adalah:

```text
check add sub fac season triangle clear about help
```

### Implementasi `clear`

```c
else if (strcmp(cmd, "clear")) {
    clearScreen();
    continue;
}
```

Fungsi `clearScreen()` mengulang seluruh 80 × 25 posisi layar. Setiap posisi diisi dengan karakter spasi dan atribut warna `0x07`:

```c
for (i = 0; i < 80 * 25; i++) {
    putInMemory(0xB800, i * 2, ' ');
    putInMemory(0xB800, i * 2 + 1, 0x07);
}
```

Setelah itu cursor dikembalikan ke awal:

```c
cursor = 0;
```

Penggunaan `continue` setelah `clearScreen()` membuat loop langsung kembali ke awal tanpa memanggil `newline()` di akhir iterasi, sehingga prompt `> ` langsung muncul di pojok kiri atas layar yang sudah bersih.

### Implementasi `help`

```c
else if (strcmp(cmd, "help")) {
    printString("check add sub fac season triangle clear about help");
}
```

Daftar command yang ditampilkan mencakup `help` itu sendiri, sehingga pengguna tahu bahwa command tersebut tersedia.

### Command `about`

Di dalam daftar help, soal juga mencantumkan `about`. Karena itu, command `about` dibuat agar ketika user mengetiknya, sistem menampilkan informasi singkat tentang OS Shell ini:

```c
else if (strcmp(cmd, "about")) {
    printString("OS Shell - Final Challenge Modul 5");
}
```

### Command Tidak Dikenal

Selain semua command di atas, sistem juga menangani input yang tidak dikenali dengan menampilkan pesan:

```c
else {
    printString("unknown command");
}
```

Hal ini memastikan shell selalu memberikan feedback kepada pengguna meskipun command yang dimasukkan tidak valid.
