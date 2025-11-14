//========================================================//
//  predictor.h                                           //
//  Header file for the Branch Predictor                  //
//                                                        //
//  Includes function prototypes and global predictor     //
//  variables and defines                                 //
//========================================================//

#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <stdint.h>
#include <stdlib.h>

//
// Student Information
//
extern const char *studentName;
extern const char *studentID;
extern const char *email;

//------------------------------------//
//      Global Predictor Defines      //
//------------------------------------//
#define NOTTAKEN 0
#define TAKEN 1

// The Different Predictor Types
#define STATIC 0
#define GSHARE 1
#define TOURNAMENT 2
#define CUSTOM 3
extern const char *bpName[];

// Definitions for 2-bit counters
#define SN 0 // predict NT, strong not taken
#define WN 1 // predict NT, weak not taken
#define WT 2 // predict T, weak taken
#define ST 3 // predict T, strong taken

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//
extern int ghistoryBits; // Number of bits used for Global History
extern int lhistoryBits; // Number of bits used for Local History
extern int pcIndexBits;  // Number of bits used for PC index
extern int bpType;       // Branch Prediction Type
extern int verbose;

//------------------------------------//
//    Predictor Function Prototypes   //
//------------------------------------//

// Initialize the predictor
//
void init_predictor();

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct);

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct);

// Please add your code below, and DO NOT MODIFY ANY OF THE CODE ABOVE
// 

void init_tournament();
//initialize data structures for tournament predictor

uint8_t tournament_predict(uint32_t pc);

void train_tournament(uint32_t pc, uint8_t outcome, int global_pred, int local_pred);

uint8_t inc_ctr(uint8_t state);

uint8_t dec_ctr(uint8_t state);

uint8_t cptupdater(uint8_t state, int lpred, int gpred);

void free_tournament();

//3 bit counter definitions
#define NNN 0 //strong not taken
#define NNT 1 
#define NTN 2
#define NTT 3
#define TNN 4
#define TNT 5
#define TTN 6
#define TTT 7 //strong taken

//TAGE definitions
#define TAGE_NUM_TABLE 6

uint32_t hash(uint64_t history, int len, int target_len);
uint32_t tage_index(uint32_t pc, uint64_t history, int len);
uint32_t tage_tag(uint32_t pc, uint64_t history, int len);

void init_tage();
uint8_t tage_predict(uint32_t pc);

void train_tage(uint32_t pc, uint8_t outcome, int main_pred_table, int alt_pred_table);

void free_tage();
uint8_t three_pred(uint8_t state);
uint8_t tage_update_counter(uint8_t counter, uint8_t outcome);
uint8_t tage_update_useful(uint8_t useful, int increment);

uint8_t two_pred(uint8_t state);

#endif
