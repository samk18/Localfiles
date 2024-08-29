using System;
using System.Collections.Generic;
using System.Linq;

namespace ProblemASolution
{
    class Program
    {
        static void Main(string[] args)
        {

              
              string ln = string.Empty;
              var Queuelines = new Queue<string>();
                    
                        while (!false)
                        {
                            ln = Console.ReadLine();
                            if (!String.IsNullOrEmpty(ln))
                            {

                            }
                            else
                            {
                                Queuelines.Enqueue(ln);
                                ln = Console.ReadLine();
                                if (!string.IsNullOrEmpty(ln))
                                {

                                }
                                else
                                {
                                    break;
                                }
                            }
                            Queuelines.Enqueue(ln);
                        }
                    

                    try {

                        string line = string.Empty;
                        int Lengthline = 0;
                        int indexlast = 0;
                        int LineInstars = 0;
                        int totalStars = 0;
                        int boo=1;
                        while (Queuelines.Any() && boo==1)
                        {

                            line = Queuelines.Dequeue();

                            if (!(!String.IsNullOrEmpty(line) && line.Length > 0))
                            {

                                Lengthline = 0;
                                indexlast = 0;
                                LineInstars = 0;
                                totalStars = 0;

                                if (Queuelines.Count > 0)
                                {
                                    Console.WriteLine();
                                }
                            }
                            else
                            {
                               Lengthline = line.Length;
                                indexlast = Lengthline - 1 - totalStars;
                                LineInstars = line.Count(x => x == '*');
                                totalStars += LineInstars;
                                var rangeFrom = indexlast - LineInstars + 1;

                                var indexesAvailable = Enumerable.Range(rangeFrom, LineInstars).ToDictionary(x => x);

                                for (int i = 1; i <= Lengthline; i++)
                                {
                                    if (!indexesAvailable.ContainsKey(i-1))
                                    {
                                        Console.Write("."); 
                                    }
                                    else
                                    {
                                        Console.Write("*");
                                    }
                                }

                                Console.WriteLine();


                            }

                        }


                    }
                    catch (NullReferenceException e)
                    {
                        Console.WriteLine("Output line");
                        Console.WriteLine(e.Message);
                    }
            
        }
    }
}