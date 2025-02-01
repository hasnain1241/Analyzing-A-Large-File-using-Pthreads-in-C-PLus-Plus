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
pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
unordered_map<string, int> globalWordFrequency;         // Global map to store word frequencies
int globalWordCount = 0;                                // Total number of words
int globalVowelWordCount = 0;                           // Count of words starting with vowels

// Function to process a chunk of text
void* analyzeChunk(void* arg) {
    string* textSegment = static_cast<string*>(arg);    // Convert argument to string pointer
    unordered_map<string, int> localWordFrequency;      // Local map for word counts
    int localWordCount = 0;                             // Local word count for this chunk
    int localVowelWordCount = 0;                        // Count of words starting with vowels in this chunk

    string tempWord; // Temporary variable to build words
    for (char character : *textSegment) {
        if (isalpha(character)) {
            tempWord += tolower(character); // Convert to lowercase and build the word
        } else if (!tempWord.empty()) {
            // Process the completed word
            localWordCount++; // Increment total word count
            if (tempWord[0] == 'a' || tempWord[0] == 'e' || tempWord[0] == 'i' || tempWord[0] == 'o' || tempWord[0] == 'u') {
                localVowelWordCount++; // Check if the word starts with a vowel
            }
            localWordFrequency[tempWord]++; // Increment frequency in the local map
            tempWord.clear(); // Clear the word for the next one
        }
    }

    // Handle the last word if the chunk ends with one
    if (!tempWord.empty()) {
        localWordCount++;
        if (tempWord[0] == 'a' || tempWord[0] == 'e' || tempWord[0] == 'i' || tempWord[0] == 'o' || tempWord[0] == 'u') {
            localVowelWordCount++;
        }
        localWordFrequency[tempWord]++;
    }

    // Update global counts (thread-safe using mutex)
    pthread_mutex_lock(&threadMutex);
    globalWordCount += localWordCount;
    globalVowelWordCount += localVowelWordCount;
    for (const auto& entry : localWordFrequency) {
        globalWordFrequency[entry.first] += entry.second;
    }
    pthread_mutex_unlock(&threadMutex);

    delete textSegment; // Free the dynamically allocated memory
    return nullptr;
}

// Function to split the file into chunks for processing
vector<string> divideFileIntoChunks(const string& fileName, int numberOfChunks) {
    ifstream inputFile(fileName, ios::binary | ios::ate); // Open file in binary mode
    if (!inputFile) {
        cerr << "Error opening file!" << endl;
        exit(1); // Exit if file cannot be opened
    }

    streamsize fileSize = inputFile.tellg(); // Get file size
    inputFile.seekg(0, ios::beg); // Move to the beginning of the file

    vector<string> textChunks(numberOfChunks); // Vector to store file chunks
    streamsize chunkSize = fileSize / numberOfChunks; // Calculate size of each chunk

    for (int i = 0; i < numberOfChunks; ++i) {
        streamsize startIndex = i * chunkSize;
        streamsize endIndex = (i == numberOfChunks - 1) ? fileSize : (i + 1) * chunkSize;

        // Adjust start and end to avoid breaking words
        if (i != 0) {
            inputFile.seekg(startIndex);
            while (inputFile.get() != ' ' && inputFile.tellg() < endIndex) {}
            startIndex = inputFile.tellg();
        }

        streamsize currentChunkSize = endIndex - startIndex;
        textChunks[i].resize(currentChunkSize);
        inputFile.read(&textChunks[i][0], currentChunkSize); // Read the chunk into the vector
    }

    return textChunks; // Return the chunks
}

int main(int argumentCount, char* arguments[]) {
    if (argumentCount != 2) {
        cerr << "Usage: " << arguments[0] << " <num_threads>" << endl;
        return 1;
    }

    string inputFileName = "a2.txt";        // Input file path
    int threadCount = stoi(arguments[1]);  // Number of threads to create

    // Split the file into chunks
    vector<string> dividedChunks = divideFileIntoChunks(inputFileName, threadCount);

    // Create and launch threads
    vector<pthread_t> threadHandles(threadCount); // Vector to store thread IDs
    for (int i = 0; i < threadCount; ++i) {
        string* currentChunk = new string(dividedChunks[i]); // Dynamically allocate memory for the chunk
        pthread_create(&threadHandles[i], nullptr, analyzeChunk, currentChunk); // Create a thread
    }

    auto startTime = chrono::high_resolution_clock::now(); // Start measuring time

    // Wait for all threads to finish
    for (int i = 0; i < threadCount; ++i) {
        pthread_join(threadHandles[i], nullptr); // Join threads
    }

    auto endTime = chrono::high_resolution_clock::now(); // End measuring time

    // Calculate the elapsed time in seconds
    double executionTime = chrono::duration_cast<chrono::duration<double>>(endTime - startTime).count();

    // Output results
    cout << "Total word count: " << globalWordCount << endl;
    cout << "Count of words starting with vowels: " << globalVowelWordCount << endl;

    // Find top 10 most frequent words
    vector<pair<string, int>> sortedFrequencies(globalWordFrequency.begin(), globalWordFrequency.end());
    sort(sortedFrequencies.begin(), sortedFrequencies.end(), [](const auto& a, const auto& b) {
        return a.second > b.second; // Sort by frequency in descending order
    });

    cout << "Top 10 most frequent words:" << endl;
    for (int i = 0; i < 10 && i < sortedFrequencies.size(); ++i) {
        cout << sortedFrequencies[i].first << ": " << sortedFrequencies[i].second << endl;
    }

    cout << "Execution Time: " << executionTime << " seconds" << endl;

    // Clean up
    pthread_mutex_destroy(&threadMutex);

    return 0;
}

