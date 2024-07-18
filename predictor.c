#define BIMODAL_TABLE_SIZE 512
#define GLOBAL_HISTORY_BITS 15
#define GLOBAL_TABLE_SIZE (1 << GLOBAL_HISTORY_BITS)  // Example: 2^12 = 4096
#define CHOICE_TABLE_SIZE BIMODAL_TABLE_SIZE  // Same size as the bimodal table for simplicity
#define MAX_COUNTER 3

#include <stdint.h>
#include <stdio.h>
#include <string.h> // For memset

int bimodal_table[BIMODAL_TABLE_SIZE];
int global_table[GLOBAL_TABLE_SIZE];
int choice_table[CHOICE_TABLE_SIZE];  // Determines whether to use bimodal or global prediction
uint32_t global_history = 0;  // Tracks the global history of branch outcomes

uint64_t total_predictions = 0;
uint64_t correct_predictions = 0;
int i = 0;

// Initializes all predictor components
extern "C" void initialize_branch_predictor() {
    memset(bimodal_table, 0, sizeof(bimodal_table));
    memset(global_table, 0, sizeof(global_table));
    memset(choice_table, 0, sizeof(choice_table));
    global_history = 0;
}


// Predicts branch outcome using combined bimodal and gshare predictors
extern "C" void predict_branch(unsigned long long ip, unsigned long long hist, unsigned char *pred) {
    uint32_t bimodal_index = ip % BIMODAL_TABLE_SIZE;
    // For gshare, modify global index calculation to XOR global history with branch address bits
    uint32_t global_index = (global_history ^ (ip % GLOBAL_TABLE_SIZE)) % GLOBAL_TABLE_SIZE;
    uint32_t choice_index = ip % CHOICE_TABLE_SIZE;

    bool bimodal_prediction = bimodal_table[bimodal_index] >= 2;  // Threshold for 2-bit counter
    bool global_prediction = global_table[global_index] >= 2;

    // Use choice predictor to decide between bimodal and global
    bool use_bimodal = choice_table[choice_index] > 1;
    *pred = use_bimodal ? bimodal_prediction : global_prediction;
}

// Updates predictor states based on actual branch outcome
extern "C" void update_branch(unsigned long long ip, unsigned long long hist, unsigned char taken) {
    uint32_t bimodal_index = ip % BIMODAL_TABLE_SIZE;
    // For gshare, modify global index calculation to XOR global history with branch address bits
    uint32_t global_index = (global_history ^ (ip % GLOBAL_TABLE_SIZE)) % GLOBAL_TABLE_SIZE;
    uint32_t choice_index = ip % CHOICE_TABLE_SIZE;

    bool bimodal_prediction = bimodal_table[bimodal_index] > 1;
    bool global_prediction = global_table[global_index] > 1;

    // Decide again based on the choice predictor (simulate prediction step)
    bool use_bimodal = choice_table[choice_index] > 1;
    bool final_prediction = use_bimodal ? bimodal_prediction : global_prediction;

       // Update accuracy tracking
    total_predictions++;
    if (final_prediction == taken) {
        correct_predictions++;
    }

    // Update bimodal predictor
    if (taken) {
        if (bimodal_table[bimodal_index] < MAX_COUNTER) bimodal_table[bimodal_index]++;
    } else {
        if (bimodal_table[bimodal_index] > 0) bimodal_table[bimodal_index]--;
    }

    // Update global predictor
    if (taken) {
        if (global_table[global_index] < MAX_COUNTER) global_table[global_index]++;
    } else {
        if (global_table[global_index] > 0) global_table[global_index]--;
    }

    // Update choice predictor based on the accuracy of predictions
    if (bimodal_prediction != taken && global_prediction == taken) {
        if (choice_table[choice_index] < MAX_COUNTER) choice_table[choice_index]++;
    } else if (bimodal_prediction == taken && global_prediction != taken) {
        if (choice_table[choice_index] > 0) choice_table[choice_index]--;
    }
    // Update global history
    global_history = ((global_history << 1) | (taken ? 1 : 0)) & ((1 << GLOBAL_HISTORY_BITS) - 1);
   
}