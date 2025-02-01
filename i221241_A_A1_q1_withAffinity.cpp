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
#include <sched.h> // For CPU affinity
#include <chrono> // For high-resolution clock

using namespace std;

// Global data structures
pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
unordered_map<string, int> globalWordFrequency; // Global word frequency map
int globalWordCount = 0; // Total word count
int globalVowelWordCount = 0; // Count of words starting with vowels

// Structure to pass multiple arguments to the thread function
struct ThreadArguments {
    string* textSegment; // Pointer to the chunk of text to process
    int cpuCoreId; // CPU core ID for thread affinity
};

// Function to process a chunk
void* analyzeChunk(void* arg) {
    ThreadArguments* args = static_cast<ThreadArguments*>(arg);
    string* textSegment = args->textSegment;
    int cpuCoreId = args->cpuCoreId;

    // Set CPU affinity for the thread
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpuCoreId, &cpuSet);
    pthread_t currentThread = pthread_self();
    if (pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuSet) != 0) {
        cerr << "Failed to set thread affinity for core " << cpuCoreId << endl;
    }

    unordered_map<string, int> localWordFrequency; // Local word frequency map
    int localWordCount = 0; // Local total word count
    int localVowelWordCount = 0; // Local count of words starting with vowels

    string tempWord; // Temporary variable to store each word
    for (char character : *textSegment) {
        if (isalpha(character)) {
            tempWord += tolower(character); // Convert to lowercase and build the word
        } else if (!tempWord.empty()) {
            // If a word is complete, update local counts
            localWordCount++;
            if (tempWord[0] == 'a' || tempWord[0] == 'e' || tempWord[0] == 'i' || tempWord[0] == 'o' || tempWord[0] == 'u') {
                localVowelWordCount++;
            }
            localWordFrequency[tempWord]++;
            tempWord.clear(); // Reset for the next word
        }
    }

    // Handle the last word if the chunk ends with a word
    if (!tempWord.empty()) {
        localWordCount++;
        if (tempWord[0] == 'a' || tempWord[0] == 'e' || tempWord[0] == 'i' || tempWord[0] == 'o' || tempWord[0] == 'u') {
            localVowelWordCount++;
        }
        localWordFrequency[tempWord]++;
    }

    // Update global counts with thread safety
    pthread_mutex_lock(&threadMutex);
    globalWordCount += localWordCount;
    globalVowelWordCount += localVowelWordCount;
    for (const auto& entry : localWordFrequency) {
        globalWordFrequency[entry.first] += entry.second;
    }
    pthread_mutex_unlock(&threadMutex);

    delete textSegment; // Free the dynamically allocated chunk
    delete args; // Free the ThreadArguments structure
    return nullptr;
}

// Function to split the file into chunks
vector<string> divideFileIntoChunks(const string& fileName, int numberOfChunks) {
    ifstream inputFile(fileName, ios::binary | ios::ate); // Open the file in binary mode and move to the end
    if (!inputFile) {
        cerr << "Error opening file!" << endl;
        exit(1);
    }

    streamsize fileSize = inputFile.tellg(); // Get the total file size
    inputFile.seekg(0, ios::beg); // Move back to the beginning of the file

    vector<string> textChunks(numberOfChunks); // Vector to store text chunks
    streamsize currentChunkSize = fileSize / numberOfChunks; // Size of each chunk

    for (int i = 0; i < numberOfChunks; ++i) {
        streamsize startIndex = i * currentChunkSize;
        streamsize endIndex = (i == numberOfChunks - 1) ? fileSize : (i + 1) * currentChunkSize;

        // Adjust start index to avoid splitting words
        if (i != 0) {
            inputFile.seekg(startIndex);
            while (inputFile.get() != ' ' && inputFile.tellg() < endIndex) {}
            startIndex = inputFile.tellg();
        }

        streamsize adjustedChunkSize = endIndex - startIndex;
        textChunks[i].resize(adjustedChunkSize);
        inputFile.read(&textChunks[i][0], adjustedChunkSize);
    }

    return textChunks;
}

int main(int argumentCount, char* arguments[]) {
    // Check if the correct number of arguments is provided
    if (argumentCount != 2) {
        cerr << "Usage: " << arguments[0] << " <num_threads>" << endl;
        return 1;
    }

    // Hardcoded file path
    string inputFileName = "a2.txt";

    // Number of threads from command-line argument
    int threadCount = stoi(arguments[1]);

    // Split the file into chunks
    vector<string> dividedChunks = divideFileIntoChunks(inputFileName, threadCount);

    // Create and launch threads
    vector<pthread_t> threadHandles(threadCount); // Thread handles
    for (int i = 0; i < threadCount; ++i) {
        ThreadArguments* args = new ThreadArguments{new string(dividedChunks[i]), i}; // Assign each thread to a different core
        pthread_create(&threadHandles[i], nullptr, analyzeChunk, args);
    }

    auto startTime = chrono::high_resolution_clock::now(); // Record the start time

    // Wait for all threads to finish
    for (int i = 0; i < threadCount; ++i) {
        pthread_join(threadHandles[i], nullptr);
    }

    auto endTime = chrono::high_resolution_clock::now(); // Record the end time

    // Calculate the elapsed time in seconds
    double executionTime = chrono::duration_cast<chrono::duration<double>>(endTime - startTime).count();

    // Output results
    cout << "Total word count: " << globalWordCount << endl;
    cout << "Count of words starting with vowels: " << globalVowelWordCount << endl;

    // Find top 10 most frequent words
    vector<pair<string, int>> sortedFrequencies(globalWordFrequency.begin(), globalWordFrequency.end());
    sort(sortedFrequencies.begin(), sortedFrequencies.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
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

