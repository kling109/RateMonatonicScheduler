/*
Name: Trevor Kling
ID: 002270716
Email: kling109@mail.chapman.edu
Course: CPSC 380 Operating Systems
Last Date Modified: 22 April 2019
Project: Rate Monatonic Scheduler
*/

int* BOARD[10];

/*
Idling work function. Designed to promote cache misses and other small errors.
*/
void doWork()
{
  int product = 1;
  for (int i = 0; i < 5; ++i)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int k = 0; k < 10; ++k)
      {
        product *= BOARD[k][i+(5*j)];
      }
    }
  }
}

/*
Main method.  Will be used to initialize and run threads.
*/
int main()
{
  for (int i = 0; i < 10; ++i)
  {
    int* row = new int[10];
    for (int j = 0; j < 10; ++j)
    {
      row[j] = 1;
    }
    BOARD[i] = row;
  }
  return 0;
}
