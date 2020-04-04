#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

/*
 * author - Willis Knox
 *
 * This program will read from an input file and encyrpt the contents and
 * write the output to an output file. Both the input and output files need to be
 * provided by the user in the command line. If entered incorrectly, the program will
 * tell the user and give them an example of how to launch the program from the command line
 *
 * This program is multithreaded. It uses 6 threads in total to encrypt. 5 of these threads
 * are created with pthread_create() in the main() function. The other is the thread that
 * the program launches on.
 *
 * There is a reader thread that reads from the input file and writes the text to an input_buffer,
 * there is an encryption thread that reads from the input_buffer, runs the characters through the
 * encryption algorithm and writes them into the output buffer. Next there is a thread that
 * counts the number of times different characters appear in the input_buffer. After that there is
 * a thread that writes the characters in the output_buffer into the output_file given by the user,
 * and finally, there is another thread that counts the number of times different characters appear
 * in the output_buffer.
 *
 * After all of these threads are complete, the program prints out stats telling the user
 * how many times each letter appeared in the input and output files.
 */

/* Shared variables between threads */
int N;
char *input_buffer;
char *output_buffer;
int input_counts[52];
int output_counts[52];
int end_of_file;

// declaration of semaphores. Will need to be able to be seen by all threads.
sem_t read_sem, write_sem, enc_sem, i_cnt_sem, o_cnt_sem;

/* End of shared variables  */

// prototyping functions. Could write main below all of these but I like main at the top.
int cmd_line_check(int cnt, char **cmds);
void *read_file(void *input_file);
void *write_file(void *output_file);
void *encyrpt_string();
void *count_input();
void *count_output();
void print_input_cnt();
void print_output_cnt();

/*
 * Main function. First calls helper function to make sure the user correctly
 * entered an input and output file. After that, it initializes a bunch of data structures
 * that the program needs to operate.
 *
 * After init is complete, it creates all of the other threads described above,
 * waits for them to complete, then calls the print functions that print out
 * the letter stats for input and output
 */
int main(int argc, char *argv[])
{
  // check command line input - if its wrong end program after telling user
  if(cmd_line_check(argc, argv))
    return 0;
    
  // assign the input_file to the first thing in cmdline, output_file to the second
  char *input_file = argv[1];
  char *output_file = argv[2];

  // prompts the user for a size for the input_buffer. Whatever the user enters
  // will be the maximum amount of characters that can be grabbed from the input file
  // at a time
  printf("Enter buffer size: ");
  scanf("%d", &N);
  
  // init a checker that keeps track of whether or not we have read the whole input file
  end_of_file = 0;
  
  // initialize the buffers to the size given by the user above.
  input_buffer = (char *) malloc(sizeof(char) * N);
  output_buffer = (char *) malloc(sizeof(char) * N);
  
  // initialize the data structures that hold the counts of the letters
  // initialize all values to 0 to start out
  memset(&input_counts[0], 0, sizeof(input_counts));
  memset(&output_counts[0], 0, sizeof(output_counts));
  
  // initialize all of our semaphores. The reader semaphore will start with value = 1
  // while all of the other start at 0.
  sem_init(&read_sem, 0, 1);
  sem_init(&write_sem, 0, 0);
  sem_init(&enc_sem, 0, 0);
  sem_init(&i_cnt_sem, 0, 0);
  sem_init(&o_cnt_sem, 0, 0);
  
  // declare all thread other than the main thread
  pthread_t reader, writer, encyrpt, input_cnt, output_cnt;
  
  // create all of the threads. reader will need the input_file
  // and writer will need the output file. The rest do not
  // need anything else.
  pthread_create(&reader, NULL, read_file, input_file);
  pthread_create(&writer, NULL, write_file, output_file);
  pthread_create(&encyrpt, NULL, encyrpt_string, NULL);
  pthread_create(&input_cnt, NULL, count_input, NULL);
  pthread_create(&output_cnt, NULL, count_output, NULL);
  
  // wait for all of the threads to finish
  pthread_join(reader, NULL);
  pthread_join(writer, NULL);
  pthread_join(encyrpt, NULL);
  pthread_join(input_cnt, NULL);
  pthread_join(output_cnt, NULL);
  
  // done with the semaphores, so destroy them
  sem_destroy(&read_sem);
  sem_destroy(&write_sem);
  sem_destroy(&enc_sem);
  sem_destroy(&i_cnt_sem);
  sem_destroy(&o_cnt_sem);
  
  // print out letter stats
  print_input_cnt();
  print_output_cnt();
  
  return 0;
}


/*
 * Function that checks to see if the user correctly gave an input and output file.
 * If they didn't, the function prints them an example of how to correctly run the program.
 */
int cmd_line_check(int cnt, char **cmds)
{
  //cmds[0] == "./encrypt"
  if (cnt != 3)
  {
    printf("This program needs the names of the input and output files you are trying to use\n\n");
    if (cnt == 2)
    {
      printf("You only gave an input file \"%s\". You need to add an output file after!\n\n", cmds[1]);
    }
    else
    {
      printf("You didn't supply any files. After \"./encrypt\", add the names of the files\nyou are wanting to read and write from.\n\n");
    }
    printf("An example command line launch of the program is:\n\n\"./encrypt input_file output_file\"\n\nThe input file must come first and must exist, otherwise it will have nothing to encrypt!\n");
    return 1;
  }
  return 0;
}



/*
 * Function that is the reader thread. Reads from the input_file and places characters
 * as it reads them into the input_buffer. Tells the encyrption thread and the input_counter_thread
 * to launch.
 * Thread immediately starts to read as soon as it is moved into. This is because its value was set
 * to 1, so the first sem_wait() doesn't stop it, it just moves its value to 0. The second time
 * it runs into the sem_wait(), it will pause and wait for a sem_post().
 */
void *read_file(void *input_file)
{
  FILE *fp = fopen(input_file, "r");
  
  int i = 0;
  int c;
  
  while (!end_of_file)
  {
    sem_wait(&read_sem);
    for (i = 0; i < N; i++)
    {
      c = fgetc(fp);
      if (c == EOF)
      {
        end_of_file = 1;
        int j;
        for (j = i; j < N; j++) // null out old chars in input_buffer
          input_buffer[j] = '\0';
        break;
      }
      input_buffer[i] = (char) c;
    }
    sem_post(&enc_sem);
    sem_post(&i_cnt_sem);
  }
  fclose(fp);
  return NULL;
}

/*
 * Function that is the writer_thread that writes from the output_buffer into the output_file.
 * After it writes, it wakes up the reader_thread.
 *
 * Only opens the output_file once. And deletes all contents from the file if it exists beforehand.
 */
void *write_file(void *output_file)
{
  
  FILE *fp = fopen(output_file, "w+");
  
  int i;
  do
  {
    sem_wait(&write_sem);
    for (i = 0; i < N; i++)
    {
      fputc(output_buffer[i], fp);
    }
    // done writing for this buffer. Tell the read_file it can keep reading with sem_post
    // wait for count_output to finish. if it finishes first, the wait will cancel out.
    sem_wait(&write_sem);
    sem_post(&read_sem);
  } while (!end_of_file);
  fclose(fp);
  return NULL;
}


/*
 * Function that is the encyrption thread. Looks at the input_buffer characters
 * and runs them through the encryption algorithm described in the PDF.
 * It writes the encrypted chars to the output_buffer for the writer thread
 * and output_counter_thread to use. After encrypting, it lets those threads
 * go to work.
 */
void *encyrpt_string()
{
  int s = 1;
  int i;
  
  do
  {
    sem_wait(&enc_sem);
    for (i = 0; i < N; i++)
    {
      char ch = input_buffer[i];
      
      if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
      {
        if (s == 1) // inc char by 1 w/ wraparound
        {
          if (ch - 'Z' == 0)
            output_buffer[i] = 'A';
          else if (ch - 'z' == 0)
            output_buffer[i] = 'a';
          else
            output_buffer[i] = ch + 1;
          s = -1;
        }
        else if (s == -1) // dec char by 1 w/ wraparound
        {
          if (ch - 'A' == 0)
            output_buffer[i] = 'Z';
          else if (ch - 'a' == 0)
            output_buffer[i] = 'z';
          else
            output_buffer[i] = ch - 1;
          s = 0;
        }
        else // keep char the same
        {
          output_buffer[i] = ch;
          s = 1;
        }
      }
      else // wasn't a letter, so keep char the same
        output_buffer[i] = ch;
    }
    // tell writer and output counter to startup
    sem_post(&write_sem);
    sem_post(&o_cnt_sem);
  } while (!end_of_file);
  return NULL;
}

/*
 * Function that is the thread that keeps track of the number of times
 * a letter appears in the input_file. Reads from the input_buffer.
 *
 * Uses fun char to int math ASCII tricks to correctly count the letters.
 */
void *count_input()
{
  int i;
  do
  {
    sem_wait(&i_cnt_sem);
    for (i = 0; i < N; i++)
    {
      char c = input_buffer[i]; // get the char
      int x;
      if ((c >= 'A' && c <= 'Z'))
      {
        x = c - 'A'; // subtracts 'A' from the char and then converts to its int number
        ++input_counts[x]; // 'A' - 'A' == 0, so inc input_counts[0] by 1
      }
      else if ((c >= 'a' && c <= 'z'))
      {
        x = c - 'a'; // subtracts 'a' from the char and then converts to its int number
        x += 26; // add 26 because lower case letters are in positions 26-51 in input_counts
        ++input_counts[x]; // 'a' - 'a' == 0, so inc input_counts[26] by 1
      }
    }
  } while (!end_of_file);
  return NULL;
}


/*
 * Function that is the thread that keeps track of the number of times
 * a letter appears in the output_file. Reads from the output_buffer.
 *
 * Uses fun char to int math ASCII tricks to correctly count the letters.
 */
void *count_output()
{
  int i;
  do
  {
    sem_wait(&o_cnt_sem);
    for (i = 0; i < N; i++)
    {
      char c = output_buffer[i];
      int x;
      if ((c >= 'A' && c <= 'Z'))
      {
        x = c - 'A'; // subtracts 'A' from the char and then converts to its int number
        ++output_counts[x]; // 'a' - 'a' == 0, so inc output_counts[26] by 1
      }
      else if ((c >= 'a' && c <= 'z'))
      {
        x = c - 'a'; // subtracts 'a' from the char and then converts to its int number
        x += 26; // add 26 because lower case letters are in positions 26-51 in output_counts
        ++output_counts[x]; // 'a' - 'a' == 0, so inc output_counts[26] by 1
      }
    }
    // adds 1 to write_sem.value incase write thread finished before this thread
    // this helps make sure the threads don't get cross over and mess with each other
    sem_post(&write_sem);
  } while (!end_of_file);
  
  return NULL;
}

/*
 * Helper function that prints out the input stats for the letters.
 * For ease of reading. It prints out upper and lowercase on the same line.
 * So the stats for 'A' and 'a' will be on one line. The next line will be
 * 'B' and 'b' ... and so forth
 *
 * Converts i to a char and adds ASCII constants 65 and 97 because
 * 65 == 'A' and 97 == 'a'
 */
void print_input_cnt()
{
  printf("------------------------------------------\n");
  printf("Input file contains:\n");
  int i;
  for (i = 0; i < 26; i++)
  {
    printf("%c: %d \t\t %c: %d\n", (i - 0) + 65, input_counts[i],
                                   (i - 0) + 97, input_counts[i + 26]);
  }
  printf("------------------------------------------\n");
}


/*
 * Helper function that prints out the output stats for the letters.
 * For ease of reading. It prints out upper and lowercase on the same line.
 * So the stats for 'A' and 'a' will be on one line. The next line will be
 * 'B' and 'b' ... and so forth
 *
 * Converts i to a char and adds ASCII constants 65 and 97 because
 * 65 == 'A' and 97 == 'a'
 */
void print_output_cnt()
{
  printf("------------------------------------------\n");
  printf("Output file contains:\n");
  int i;
  for (i = 0; i < 26; i++)
  {
    printf("%c: %d \t\t %c: %d\n", (i - 0) + 65, output_counts[i],
                                   (i - 0) + 97, output_counts[i + 26]);
  }
  printf("------------------------------------------\n");
}
