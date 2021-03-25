#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include<fstream>
#include <bits/stdc++.h>

#define MCW MPI_COMM_WORLD

using namespace std;

vector <string> readEachWord(string filename) {
    fstream file;
    string word;
    file.open(filename.c_str());

    vector <string> words;

    while (file >> word) {
        words.push_back(word);
    }
    file.close();

    return words;
}

void writeToFile(vector <string> words) {
    string filename("corrected.md");
    fstream fout;

    fout.open(filename, std::ios_base::out);
    for (int i = 0; i < words.size(); i++) {
        fout << words.at(i) << " ";
    }
}

int minDistance(string actualString, string testString) {
    transform(actualString.begin(), actualString.end(), actualString.begin(), ::tolower);
    actualString.erase(remove_if(actualString.begin(), actualString.end(), [](char c) { return !isalpha(c); } ), actualString.end());

    int n = actualString.size();
    int m = testString.size();
    int **dp = new int *[n + 1];
    for (int i = 0; i <= n; i++) {
        dp[i] = new int[m + 1];
        for (int j = 0; j <= m; j++) {
            dp[i][j] = 0;
            if (i == 0)dp[i][j] = j;
            else if (j == 0)dp[i][j] = i;
        }
    }
    actualString = " " + actualString;
    testString = " " + testString;

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            if (actualString[i] != testString[j]) {
                dp[i][j] = 1 + min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            } else {
                dp[i][j] = dp[i - 1][j - 1];
            }
        }
    }
    return dp[n][m];
}

string getCorrectWordOrCorrection(string s, vector <string> dictionary) {
    int best = 5;
    map<string, int> closestWords;

    for (int i = 0; i < dictionary.size(); i++) {
        int dist = minDistance(s, dictionary.at(i));

        if (dist < best) {
            best = dist;
            closestWords.insert(pair<string, int>(dictionary.at(i), dist));
        }
        else if(abs(best - dist) < 2) {
            closestWords.insert(pair<string, int>(dictionary.at(i), dist));
        }

        if (best == 0) {
            return s;
        }
    }

    map<string, int>::iterator it;
    int closest = 5;

    for(it = closestWords.begin(); it != closestWords.end(); it++) {
        if(it->second < closest) {
            closest = it->second;
        }
    }

    string correctedString = "*";
    correctedString += s;

    correctedString += "* ***(";
    int count = 0;
    for(it = closestWords.begin(); it != closestWords.end(); it++) {
        if(it->second == closest) {
            correctedString += it->first;
            count++;

            if(count == 5) {
                correctedString += ")***";
                return correctedString;
            }

            correctedString += "|";
        }
    }

    correctedString.pop_back();
    correctedString += ")***";

    return correctedString;
}



int main(int argc, char **argv) {

    int rank, size;
    int DIE = 0;

    MPI_Status mystatus;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank);
    MPI_Comm_size(MCW, &size);
    srand(rank * time(0));

    if (!rank) {
        vector <string> words = readEachWord("bad_file.txt");
        cout << words.size() << endl;
        for (int i = 0; i < words.size(); i++) {
            int dest;
            do {
                dest = (rand() % size);
            } while (dest == 0);

            MPI_Send(words.at(i).c_str(), words.at(i).size() + 1, MPI_CHAR, dest, i + 1, MCW);
        }

        vector <string> correct;
        for (int i = 1; i < words.size() + 1; i++) {
            char data[128];
            MPI_Recv(&data, 128, MPI_CHAR, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &mystatus);
            correct.push_back(string(data));
        }

        writeToFile(correct);

        for (int i = 1; i < size; i++) {
            char data[] = "die";
            MPI_Send(&data, 50, MPI_CHAR, i, DIE, MCW);
        }
    } else {
        vector <string> dictionary = readEachWord("popular.txt");
        while (true) {
            int myflag;
            char data[128];
            MPI_Recv(&data, 128, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mystatus);

            if (mystatus.MPI_TAG == DIE) {
                break;
            }

            string word(data);

            string corrected = getCorrectWordOrCorrection(word, dictionary);

            MPI_Send(corrected.c_str(), 128, MPI_CHAR, 0, mystatus.MPI_TAG, MCW);
        }
    }

    MPI_Finalize();

    return 0;
}
