/*

Muhammad Hasnain Akhtar
CS-6A
22i-1241

Assignment 1


*/


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <pthread.h>
#include <cctype>
#include <chrono> // For high-resolution clock

using namespace std;

// Global data structures
pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
unordered_map<string, int> globalWordFrequency; // Global word frequency map (hash table)
int globalTotalWords = 0; // Total word count
int totalUniqueWords = 0; // Total number of unique words

// Function to process a chunk
void* chunkProcessor(void* arguments) {
    string* dataChunk = static_cast<string*>(arguments);
    unordered_map<string, int> localWordFrequency;
    int localWordCount = 0;

    string currentWord;
    for (char currentChar : *dataChunk) {
        if (isalpha(currentChar)) {
            currentWord += tolower(currentChar); // Convert to lowercase
        } else if (!currentWord.empty()) {
            // Update local counts
            localWordCount++;
            localWordFrequency[currentWord]++;
            currentWord.clear();
        }
    }

    // Handle the last word if the chunk ends with a word
    if (!currentWord.empty()) {
        localWordCount++;
        localWordFrequency[currentWord]++;
    }

    // Update global counts with thread safety
    pthread_mutex_lock(&globalMutex);
    globalTotalWords += localWordCount;
    for (const auto& word : localWordFrequency) {
        globalWordFrequency[word.first] += word.second;
    }
    pthread_mutex_unlock(&globalMutex);

    delete dataChunk; // Free the dynamically allocated chunk
    return nullptr;
}

// Function to split the file into chunks
vector<string> fileChunkSplitter(const string& filePath, int numberOfChunks) {
    ifstream inputFile(filePath, ios::binary | ios::ate);
    if (!inputFile) {
        cerr << "Error opening file!" << endl;
        exit(1);
    }

    streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    vector<string> fileChunks(numberOfChunks);
    streamsize chunkSize = fileSize / numberOfChunks;

    for (int chunkIndex = 0; chunkIndex < numberOfChunks; ++chunkIndex) {
        streamsize startByte = chunkIndex * chunkSize;
        streamsize endByte = (chunkIndex == numberOfChunks - 1) ? fileSize : (chunkIndex + 1) * chunkSize;

        // Adjust start and end to avoid splitting words
        if (chunkIndex != 0) {
            inputFile.seekg(startByte);
            while (inputFile.get() != ' ' && inputFile.tellg() < endByte) {}
            startByte = inputFile.tellg();
        }

        streamsize size = endByte - startByte;
        fileChunks[chunkIndex].resize(size);
        inputFile.read(&fileChunks[chunkIndex][0], size);
    }

    return fileChunks;
}

// Function to write results to a text file
void saveResultsToFile(const string& resultFilePath) {
    ofstream resultFile(resultFilePath);
    if (!resultFile) {
        cerr << "Error creating result file!" << endl;
        exit(1);
    }

    // Sort word frequencies in descending order
    vector<pair<string, int>> sortedWordFrequency(globalWordFrequency.begin(), globalWordFrequency.end());
    sort(sortedWordFrequency.begin(), sortedWordFrequency.end(), [](const auto& firstWord, const auto& secondWord) {
        return firstWord.second > secondWord.second;
    });

    // Write results to the file
    resultFile << "Total word count: " << globalTotalWords << "\n";
    resultFile << "Total number of unique words: " << totalUniqueWords << "\n\n";
    resultFile << "Term frequency for each word (sorted by frequency):\n";
    for (const auto& word : sortedWordFrequency) {
        resultFile << word.first << ": " << word.second << "\n";
    }

    cout << "Results saved to " << resultFilePath << endl;
}

int main(int argumentCount, char* argumentValues[]) {

    if (argumentCount != 2) {
        cerr << "Usage: " << argumentValues[0] << " <number_of_threads>" << endl;
        return 1;
    }

    string inputFilePath = "a2.txt";
    int threadCount = stoi(argumentValues[1]);
 
    // Split the file into chunks
    vector<string> fileChunks = fileChunkSplitter(inputFilePath, threadCount);

    // Create and launch threads
    vector<pthread_t> processingThreads(threadCount);
    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        string* chunk = new string(fileChunks[threadIndex]); // Dynamically allocate chunk
        pthread_create(&processingThreads[threadIndex], nullptr, chunkProcessor, chunk);
    }

    auto startTime = chrono::high_resolution_clock::now();

    // Wait for all threads to finish
    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        pthread_join(processingThreads[threadIndex], nullptr);
    }
    
    auto endTime = chrono::high_resolution_clock::now();

    // Calculate the elapsed time in seconds
    double totalTimeElapsed = chrono::duration_cast<chrono::duration<double>>(endTime - startTime).count();

    // Calculate the total number of unique words
    totalUniqueWords = globalWordFrequency.size();

    // Output results to console
    cout << "Total word count: " << globalTotalWords << endl;
    cout << "Total number of unique words: " << totalUniqueWords << endl;

    // Write results to a text file
    saveResultsToFile("q2_without_affinity.txt");

    cout << "Execution Time: " << totalTimeElapsed << " seconds" << endl;

    // Clean up
    pthread_mutex_destroy(&globalMutex);

    return 0;
}

