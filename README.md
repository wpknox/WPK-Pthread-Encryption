# WPK-Pthread-Encryption

This is a program written for COM S 352 for the Spring 2020 semester. The goal of this assignment was to write a program that utilizes multiple threads to encrypt an input file and write the encrypted output to a separate output file.

To build this project, simply type `make` in the terminal, and after it has compiled, type `./encrypt "input_file_name" "output_file_name"`. Do not include the quotation marks around the file names.

The input file has to exist beforehand, as the program will not make one for you. The output file does not have to exist, but if it does, the program will overwrite what was in the file previously and then start writing at the beginning of the file.
___

After you launch the program, it will ask the user for a buffer size. Whatever number the user enters will be the number of characters the program grabs from the input file at a time. After it grabs up to *N* characters, it will encrypt the characters, count the number of occurences of each letter in the input file, count the number of occurences of each enrypted letter in the output file, and write to the output file before grabbing the next set of *N* characters from the specified input file.
___

This program is **not** maximumly concurrent. It does utilize the *pthreads* and *semaphores* libraries, but it will not be able to encrypt one character and read another character at the same time.

___

The encryption algorithm is fairly simple, the following is the pseudocode for the algorithm (from project description):
```
1. s = 1.
2. Get next character c.
3. if c is not a letter then goto (7).
4. if (s == 1) then increase c with wraparound (e.g., 'A' becomes 'B', 'c' becomes 'd', 'Z' becomes 'A', 'z' becomes 'a'), set s = -1, and goto (7).
5. if (s == -1) then decrease c with wraparound (e.g., 'B' becomes 'A', 'd' becomes 'c', 'A' becomes 'Z', 'a' becomes 'z'), set s=0, and goto (7).
6. if (s == 0), then do not change c, and set s=1.
7. encrypted character is c.
8. if (c != EOF) then goto (2).
```

Obviously, this is an easy algorithm to crack, but the main purpose of the project was to get more familiar with multithreaded programming and its uses.

