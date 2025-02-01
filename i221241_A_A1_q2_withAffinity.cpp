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
#include <chrono> // For measuring execution time

using namespace std;

// Global variables and data structures
pthread_mutex_t dataLock = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronizing shared data
unordered_map<string, int> wordFrequency; // Global word count storage
int totalWords = 0; // Tracks total number of words processed
int uniqueWords = 0; // Counts unique words in the text

// Structure to store thread data
struct ThreadArgs {
    string * textChunk; // Pointer to a chunk of text
    int processorID;    // Core ID for thread affinity
};

// Function to process a text chunk in a separate thread
void* analyzeTextChunk(void* arg) {
    ThreadArgs* threadData = static_cast<ThreadArgs*>(arg);
    string * textSegment = threadData->textChunk;
    int cpuCore = threadData->processorID;

    // Set thread affinity to a specific CPU core
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpuCore, &cpuSet);
    pthread_t currentThread = pthread_self();
    
    if (pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuSet) != 0) {
        cerr << "Failed to assign thread to core " << cpuCore << endl;
    }

    unordered_map<string, int> localWordCount; // Local word frequency storage
    int localTotalWords = 0; // Local word counter

    string currentWord;
    int i = 0;
    while (i < textSegment->size()) {
        char ch = (*textSegment)[i];
        if (isalpha(ch)) {
            currentWord += tolower(ch); // Convert to lowercase and accumulate letters
        } else if (!currentWord.empty()) {
            localTotalWords++; // Increase total word count
            localWordCount[currentWord]++; // Store word in local count
            currentWord.clear(); // Reset for next word
        }
        i++;
    }

    // Handle any last word if chunk ends with an unfinished word
    if (!currentWord.empty()) {
        localTotalWords++;
        localWordCount[currentWord]++;
    }

    // Merge local results into global data with thread safety
    pthread_mutex_lock(&dataLock);
    totalWords += localTotalWords;
    for (const auto& entry : localWordCount) {
        wordFrequency[entry.first] += entry.second;
    }
    pthread_mutex_unlock(&dataLock);

    delete textSegment; // Free allocated memory
    delete threadData;
    return nullptr;
}

// Function to divide the input file into text chunks for parallel processing
vector<string> divideFileIntoChunks(const string& filePath, int numChunks) {
    ifstream inputFile(filePath, ios::binary | ios::ate);
    if (!inputFile) {
        cerr << "Error opening file!" << endl;
        exit(1);
    }

    streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    vector<string> textChunks(numChunks);
    streamsize chunkSize = fileSize / numChunks;

    int chunkIndex = 0;
    while (chunkIndex < numChunks) {
        streamsize start = chunkIndex * chunkSize;
        streamsize end = (chunkIndex == numChunks - 1) ? fileSize : (chunkIndex + 1) * chunkSize;

        // Adjust start position to avoid splitting words in half
        if (chunkIndex != 0) {
            inputFile.seekg(start);
            while (inputFile.get() != ' ' && inputFile.tellg() < end) {}
            start = inputFile.tellg(); // Move to the next word boundary
        }

        streamsize size = end - start;
        textChunks[chunkIndex].resize(size);
        inputFile.read(&textChunks[chunkIndex][0], size);
        
        chunkIndex++;
    }

    return textChunks;
}

// Function to write word frequency analysis results to a file
void saveResultsToFile(const string& outputFilePath) {
    ofstream outputFile(outputFilePath);
    if (!outputFile) {
        cerr << "Error creating output file!" << endl;
        exit(1);
    }

    // Convert unordered_map to vector for sorting based on frequency
    vector<pair<string, int>> sortedWords(wordFrequency.begin(), wordFrequency.end());
    sort(sortedWords.begin(), sortedWords.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Write results to output file
    outputFile << "Total word count: " << totalWords << "\n";
    outputFile << "Total number of unique words: " << uniqueWords << "\n\n";
    outputFile << "Word Frequency (Sorted by Occurrence):\n";

    int i = 0;
    while (i < sortedWords.size()) {
        outputFile << sortedWords[i].first << ": " << sortedWords[i].second << "\n";
        i++;
    }

    cout << "Results saved to " << outputFilePath << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <num_threads>" << endl;
        return 1;
    }

    string inputFilePath = "a2.txt"; // Input file containing text
    int threadCount = stoi(argv[1]); // Number of threads to create

    // Start high-resolution timer
    auto startTime = chrono::high_resolution_clock::now();

    // Split file into smaller text chunks for multi-threaded processing
    vector<string> textChunks = divideFileIntoChunks(inputFilePath, threadCount);

    // Create and launch threads for parallel processing
    vector<pthread_t> workerThreads(threadCount);
    int threadIndex = 0;
    while (threadIndex < threadCount) {
        ThreadArgs* threadData = new ThreadArgs{new string(textChunks[threadIndex]), threadIndex};
        pthread_create(&workerThreads[threadIndex], nullptr, analyzeTextChunk, threadData);
        threadIndex++;
    }

    // Wait for all threads to complete execution
    int joinIndex = 0;
    do {
        pthread_join(workerThreads[joinIndex], nullptr);
        joinIndex++;
    } while (joinIndex < threadCount);

    // Stop the timer after execution
    auto endTime = chrono::high_resolution_clock::now();

    // Compute total execution time
    double elapsedSeconds = chrono::duration_cast<chrono::duration<double>>(endTime - startTime).count();

    // Calculate the total number of unique words
    uniqueWords = wordFrequency.size();

    // Display results in console
    cout << "Total word count: " << totalWords << endl;
    cout << "Total number of unique words: " << uniqueWords << endl;

    // Save results to a text file
    saveResultsToFile("output_results(affinity).txt");

    cout << "Execution Time: " << elapsedSeconds << " seconds" << endl;

    // Clean up and release mutex
    pthread_mutex_destroy(&dataLock);

    return 0;
}

