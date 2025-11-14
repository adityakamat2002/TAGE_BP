//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Aditya Kamat";
const char *studentID = "A69041463";
const char *email = "adkamat@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

//tournament
uint64_t tnmt_ghistory;
int tnmt_global_bits = 13;
int lhistoryBits =11;
int lptbits=11;
int choicebits=13;
uint64_t *lhr_tnmt;
uint8_t *lpt_tnmt;
uint8_t *ghr_tnmt;
uint8_t *cpt_tnmt;
uint8_t global_pred=WN;
uint8_t local_pred=NTT;
uint8_t choice=WN;
//ghr size = 2^13 x2 bits= 16KB
//lhr size = 2^11 x11 bits= 22KB
//lpt size = 2^11 x3 bits= 6KB
//cpt size = 2^13 x2 bits= 16KB
//total = 60KB < 64KB + 1024B

//tage
int table_match,alt_match;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}


//TAGE Declarations
void init_tage();
uint8_t tage_predict(uint32_t pc);

void train_tage(uint32_t pc, uint8_t outcome, int main_pred_table, int alt_pred_table);

void free_tage();
uint8_t three_pred(uint8_t state);
uint8_t tage_update_counter(uint8_t counter, uint8_t outcome);
uint8_t tage_update_useful(uint8_t useful, int increment);



void cleanup_gshare()
{
  free(bht_gshare);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    init_tage();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return tage_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome, global_pred, local_pred);
    case CUSTOM:
      return train_tage(pc,outcome,table_match,alt_match);
    default:
      break;
    }
  }
}

void init_tournament()
{
  uint32_t ghr_entries = 1 << tnmt_global_bits;
  uint32_t lhr_entries = 1 << lhistoryBits;
  uint32_t lpt_entries = 1 << lptbits;
  uint32_t cpt_entries = 1 << choicebits;

  lhr_tnmt = (uint64_t *)malloc(lhr_entries * sizeof(uint64_t));
  ghr_tnmt = (uint8_t *)malloc(ghr_entries * sizeof(uint8_t));
  lpt_tnmt = (uint8_t *)malloc(lpt_entries * sizeof(uint8_t));
  cpt_tnmt = (uint8_t *)malloc(cpt_entries * sizeof(uint8_t));

  int i;
  for(i=0;i<lpt_entries;i++)
  {
    lpt_tnmt[i] = TNN;
  }
  for(i=0;i<ghr_entries;i++)
  {
    ghr_tnmt[i] = WN;
  }
  for(i=0;i<cpt_entries;i++)
  {
    cpt_tnmt[i] = WN;
  }
  for(i=0;i<lhr_entries;i++)
  {
    lhr_tnmt[i] = 0;
  } 
  tnmt_ghistory=0;
}

uint8_t tournament_predict(uint32_t pc)
{
  uint32_t ghr_entries = 1 << tnmt_global_bits;
  uint32_t lhr_entries = 1 << lhistoryBits;
  uint32_t lpt_entries = 1 << lptbits;
  uint32_t cpt_entries = 1 << choicebits;

  //u_int8_t local_pred;
  uint32_t pc_index = pc & (lhr_entries - 1);
  //uint64_t lpt_index = lhr_tnmt[pc_index];
  //uint32_t pc_index = pc & (lhr_entries - 1);
  uint64_t local_history = lhr_tnmt[pc_index];
  uint64_t lpt_index = local_history & (lpt_entries - 1); //obtain index to address lpt
  
  switch(lpt_tnmt[lpt_index]){
    case NNN:
    local_pred = NOTTAKEN;
    break;
  case NNT:
    local_pred = NOTTAKEN;
    break;
  case NTN:
    local_pred = NOTTAKEN;
    break;
  case NTT:
    local_pred = NOTTAKEN;
    break;
  case TNN:
    local_pred = TAKEN;
    break;
  case TNT:
    local_pred = TAKEN;
    break;
  case TTN:
    local_pred = TAKEN;
    break;
  case TTT:
    local_pred = TAKEN;
    break;     
  default:
    printf("Warning: Undefined state of entry in TNMT LPT!\n");
    local_pred = NOTTAKEN;
    break;
  }
  uint32_t ghr_index = tnmt_ghistory & (ghr_entries - 1);
  //uint8_t global_pred,choice;
  switch (ghr_tnmt[ghr_index])
  {
  case WN:
    global_pred = NOTTAKEN;
    break;
  case SN:
    global_pred = NOTTAKEN;
    break;
  case WT:
    global_pred = TAKEN;
    break;
  case ST:
    global_pred = TAKEN;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    global_pred = NOTTAKEN;
    break;
  }

  uint32_t pc_lower_bits = pc & (cpt_entries - 1);
  uint32_t tnmt_ghistory_lower_bits = tnmt_ghistory & (cpt_entries - 1);
  uint32_t cpt_index = pc_lower_bits ^ tnmt_ghistory_lower_bits;
  switch (cpt_tnmt[cpt_index])
  {
  case WN:
    choice = NOTTAKEN;
    break;
  case SN:
    choice = NOTTAKEN;
    break;
  case WT:
    choice = TAKEN;
    break;
  case ST:
    choice = TAKEN;
    break;
  default:
    printf("Warning: Undefined state of entry in TNMT CPT!\n");
    choice = NOTTAKEN;
    break;
  }

  uint8_t pred = (choice==TAKEN)?local_pred:global_pred;
  return pred;
}

void train_tournament(uint32_t pc, uint8_t outcome, int global_pred, int local_pred){
  uint32_t ghr_entries = 1 << tnmt_global_bits;
  uint32_t lhr_entries = 1 << lhistoryBits;
  uint32_t lpt_entries = 1 << lptbits;
  uint32_t cpt_entries = 1 << choicebits;

  uint32_t lhr_address = pc&(lhr_entries-1); 
  uint64_t lpt_index = lhr_tnmt[lhr_address]& (lpt_entries - 1); //obtain index to address lpt
  uint32_t ghr_index = tnmt_ghistory & (ghr_entries - 1);
  int lpmatch=(local_pred==outcome)?1:0; //does local oredictor match
  int gpmatch=(global_pred==outcome)?1:0; //does global predictor match

  switch (lpt_tnmt[lpt_index])
  {
  case NNN:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? NNT : NNN;
    break;
  case NNT:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? NTN : NNN;
    break;
  case NTN:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? NTT : NNT;
    break;
  case NTT:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? TNN : NTN;
    break;
  case TNN:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? TNT : NTT;
    break;
  case TNT:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? TTN : TNN;
    break;
  case TTN:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ? TTT : TNT;
    break;
  case TTT:
    lpt_tnmt[lpt_index] = (outcome == TAKEN) ?  TTT : TTN;
    break;    
  default:
    printf("Warning: Undefined state of entry in TNMT LPT!\n");
    break;
  }

  switch (ghr_tnmt[ghr_index])
  {
  case WN:
    ghr_tnmt[ghr_index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    ghr_tnmt[ghr_index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    ghr_tnmt[ghr_index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    ghr_tnmt[ghr_index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in TNMT GHR!\n");
    break;
  }

  uint32_t pc_lower_bits = pc & (cpt_entries - 1);
  uint32_t tnmt_ghistory_lower_bits = tnmt_ghistory & (cpt_entries - 1);
  uint32_t cpt_index = pc_lower_bits ^ tnmt_ghistory_lower_bits;
  cpt_tnmt[cpt_index]=cptupdater(cpt_tnmt[cpt_index],lpmatch,gpmatch);
  tnmt_ghistory = ((tnmt_ghistory << 1) | outcome) & ((1ULL << tnmt_global_bits) - 1);
  lhr_tnmt[lhr_address] = ((lhr_tnmt[lhr_address] << 1) | outcome) & ((1ULL << lptbits) - 1);
}

//increment counter
uint8_t inc_ctr(uint8_t state){
  int newstate;
  switch(state)
  {
    case(SN):
      newstate=WN;
      break;
    case(WN):
      newstate=WT;
      break;
    case(WT):
      newstate=ST;
      break;
    case(ST):
      newstate=ST;  
      break;
    default:
      newstate=state;
      printf("Warning: Invalid counter state!");
      break;  
  }
  return newstate;
}

uint8_t dec_ctr(uint8_t state){
  uint8_t newstate;
  switch(state)
  {
    case(SN):
      newstate=SN;
      break;
    case(WN):
      newstate=SN;
      break;
    case(WT):
      newstate=WN;
      break;
    case(ST):
      newstate=WT;  
      break;
    default:
      newstate=state;
      printf("Warning: Invalid counter state!");
      break;  
  }
  return newstate;
}

uint8_t cptupdater(uint8_t state, int lpred, int gpred)
{
  uint8_t ctr=state;
  if((gpred==1)&&(lpred==0))
    return dec_ctr(ctr);
  else if(((gpred==0)&&(lpred==1)))
    return inc_ctr(ctr);
  else
    return ctr;
}

void free_tournament(){
  free(ghr_tnmt);
  free(lhr_tnmt);
  free(lpt_tnmt);
  free(cpt_tnmt);
}

//TAGE Data Structures
int tage_hist_len=32;
int tage_base_bits=12;
int tage_table_hist_len[TAGE_NUM_TABLE]={1,2,4,8,16,32};
int tage_pred_table_bits=9;
int tage_tag_bits =13;
//int global_index_bits=10;

struct tage_table_entry{
  uint32_t tag;
  uint8_t ctr;
  uint8_t u;
};

uint64_t tage_ghistory;

uint8_t *tage_base_pred_table;
tage_table_entry **tage_tables;

//int table_match,alt_match;

int tage_counter;
uint8_t two_pred(uint8_t state);
uint8_t two_bit_update(uint8_t state, uint8_t outcome);

//custom predictor functions
uint8_t three_pred(uint8_t state);
uint8_t tage_update_counter(uint8_t counter, uint8_t outcome);
uint8_t inc_3ctr(uint8_t ctr);
uint8_t dec_3ctr(uint8_t ctr);


//hashing function to condense history into fewer bits(target_len)
uint32_t hash(uint64_t history, int len, int target_len){
  uint64_t folded = 0;
  uint64_t mask = (1ULL << target_len) - 1;
    
  for (int i = 0; i < len; i += target_len){
       folded ^= (history >> i) & mask;
  }
  return folded & mask;
}

uint32_t tage_index(uint32_t pc, uint64_t history, int len, int table_num){
  uint32_t index_bits = tage_pred_table_bits;
  uint32_t mask = (1 << index_bits) - 1;
  //PC^hashed history
  uint32_t pc_index = ((pc>>2)^table_num)&mask; //ignore L2SBs
  uint32_t hist_index = hash(history, len, index_bits);
  return (pc_index^hist_index)&mask;
}

uint32_t tage_tag(uint32_t pc, uint64_t history, int len,int table_num)
{
  uint32_t mask = (1<<tage_tag_bits)-1;
  
  uint32_t pc_tag = ((pc >> (2 + tage_pred_table_bits))^table_num) & mask;
  uint32_t hist_tag = hash(history, len, tage_tag_bits);
    
  return ((pc_tag ^ hist_tag) & mask);
}

void init_tage()
{
  //init base predictor
  uint32_t base_entries = 1<<tage_base_bits;
  tage_base_pred_table=(uint8_t *)malloc(base_entries*sizeof(uint8_t));

  int i;
  for (i=0;i<base_entries;i++){
    tage_base_pred_table[i]=WN; //init to weakly not taken
  }

  //tage_table_entry **tage_tables;

  //init tage tables
  tage_tables = (tage_table_entry **)malloc(TAGE_NUM_TABLE * sizeof(tage_table_entry *));
  uint32_t table_entries = 1<<tage_pred_table_bits;
  int j;
  for(i=0;i<TAGE_NUM_TABLE;i++){
    tage_tables[i] = (tage_table_entry *)malloc(table_entries * sizeof(tage_table_entry));
    for(j=0;j<table_entries;j++){
      tage_tables[i][j].ctr=TNN; //init counters to 011
      tage_tables[i][j].tag=0;  //init tag to 0
      tage_tables[i][j].u=0;    //init useful bit to 0
    }
  }
  tage_ghistory=0;
  tage_counter=0;

}


uint8_t tage_predict(uint32_t pc){
  uint32_t base_index = ((pc>>2) & ((1<<tage_base_bits)-1));
  uint8_t base_pred;
  switch(tage_base_pred_table[base_index]){
    case WN:
      base_pred=NOTTAKEN;
      break;
    case SN:
      base_pred=NOTTAKEN;
      break;
    case WT:
      base_pred=TAKEN;
      break;
    case ST:
      base_pred=TAKEN;
      break;
    default:
      printf("Invalid state of entry in TAGE Base Predictor table");
      break;
  }

  table_match=-1;
  alt_match=-1;
  int i;
  for(i=TAGE_NUM_TABLE-1;i>=0;i--)
  {
    uint32_t table_index=tage_index(pc,tage_ghistory,tage_table_hist_len[i],i);
    uint8_t pc_tag=tage_tag(pc,tage_ghistory,tage_table_hist_len[i],i);

    if(tage_tables[i][table_index].tag==pc_tag){
      if(table_match==-1)
        table_match=i;
      else if(alt_match==-1)
        alt_match=i;
    }
  }

  if(table_match>=0){
    uint32_t temp_index=tage_index(pc,tage_ghistory,tage_table_hist_len[table_match],table_match);
    uint8_t temp_var=three_pred(tage_tables[table_match][temp_index].ctr);
    return temp_var;
  }
  else{
    return base_pred;
  }
}

void train_tage(uint32_t pc, uint8_t outcome, int main_pred_table, int alt_pred_table){


  uint32_t base_index= (pc>>2)&((1<<tage_base_bits)-1);
  uint8_t base_pred=two_pred(tage_base_pred_table[base_index]);
  uint32_t main_index = 0;
  uint32_t alt_index = 0;
  uint8_t main_pred;
  uint8_t alt_pred;
  if(main_pred_table >= 0){
    main_index = tage_index(pc, tage_ghistory, tage_table_hist_len[main_pred_table],main_pred_table);
    main_pred = three_pred(tage_tables[main_pred_table][main_index].ctr);
  } else {
    main_pred = base_pred;
  }
  
  if(alt_pred_table >= 0){
    alt_index = tage_index(pc, tage_ghistory, tage_table_hist_len[alt_pred_table],alt_pred_table);
    alt_pred = three_pred(tage_tables[alt_pred_table][alt_index].ctr);
  } else {
    alt_pred = base_pred;
  }

  //Tage counter update
  if(main_pred_table>=0){
    tage_tables[main_pred_table][main_index].ctr=(outcome==TAKEN)?inc_3ctr(tage_tables[main_pred_table][main_index].ctr):dec_3ctr(tage_tables[main_pred_table][main_index].ctr);
  }
  else{
    tage_base_pred_table[base_index]=two_bit_update(tage_base_pred_table[base_index],outcome);
  }

  //useful bits update
  if(main_pred_table>=0){
    if((main_pred == outcome) && (alt_pred!=outcome)){
      tage_tables[main_pred_table][main_index].u = tage_update_useful(tage_tables[main_pred_table][main_index].u,1);
    }
    else if((main_pred != outcome) && (alt_pred==outcome)){
      tage_tables[main_pred_table][main_index].u = tage_update_useful(tage_tables[main_pred_table][main_index].u,0);
    }
  }

  //allocate in new table if no main_pred found
  if(main_pred != outcome)
  {
    int allocate_table=-1;

    for(int i=TAGE_NUM_TABLE-1;i>=0;i--)
    {
      if(main_pred_table==-1 || i>main_pred_table){
        uint32_t temp_index = tage_index(pc, tage_ghistory, tage_table_hist_len[i],i);
        
        if(tage_tables[i][temp_index].u==0){
          allocate_table=i;
          break;
        }
      }
    }

    if(allocate_table>=0){
      uint32_t allocate_index = tage_index(pc,tage_ghistory,tage_table_hist_len[allocate_table],allocate_table);
      uint8_t allocate_tag=tage_tag(pc,tage_ghistory,tage_table_hist_len[allocate_table],allocate_table);

      tage_tables[allocate_table][allocate_index].tag=allocate_tag;
      tage_tables[allocate_table][allocate_index].ctr=(outcome==TAKEN)? TNN:NTT;
      tage_tables[allocate_table][allocate_index].u=0;
    }
    else {
      //decrease usefulness of the entry with mismatched tag
      for (int i = main_pred_table + 1; i < TAGE_NUM_TABLE; i++) {
         uint32_t temp_index = tage_index(pc, tage_ghistory, tage_table_hist_len[i], i);
         // Use your existing function to safely decrement
         tage_tables[i][temp_index].u = tage_update_useful(tage_tables[i][temp_index].u, 0); 
      }
    }
    
  }
  tage_counter++;

  //reset to prevent stale entires
  if(tage_counter==256000){
    for(int i=0;i<TAGE_NUM_TABLE;i++){
      uint32_t table_entries = 1<<tage_pred_table_bits;
      for(int j=0;j<table_entries;j++){
        tage_tables[i][j].u=tage_tables[i][j].u >> 1; //right shift by one
      }
    }
    tage_counter=0;
  }

  tage_ghistory = (tage_ghistory<<1)|outcome;
  tage_ghistory &= ((1ULL << tage_hist_len) - 1);;
}

void free_tage(){
  free(tage_base_pred_table);
  for(int i=0;i<TAGE_NUM_TABLE;i++){
    free(tage_tables[i]);
  }
  free(tage_tables);
}


uint8_t three_pred(uint8_t state)
{
  switch(state){
    case NNN:
      return NOTTAKEN;
      break;
    case NNT:
      return NOTTAKEN;
      break;
    case NTN:
      return NOTTAKEN;
      break;
    case NTT:
      return NOTTAKEN;
      break;
    case TNN:
      return TAKEN;
      break; 
    case TNT:
      return TAKEN;
      break;  
    case TTN:
      return TAKEN;
      break;
    case TTT:
      return TAKEN;
      break;
    default:
      printf("Invalid counter state");
      return NOTTAKEN;
      break;             
  }
}

uint8_t tage_update_counter(uint8_t counter, uint8_t outcome) {
    if (outcome == TAKEN) {
        return (counter < 7) ? counter + 1 : 7; 
    } else {
        return (counter > 0) ? counter - 1 : 0; 
    }
}

uint8_t tage_update_useful(uint8_t useful, int increment) {
    if (increment) {
        return (useful < 3) ? useful + 1 : 3;
    } else {
        return (useful > 0) ? useful - 1 : 0;
    }
}

uint8_t two_pred(uint8_t state){
  uint8_t pred;
  switch(state)
  {
    case SN:
       pred = NOTTAKEN;
      break;
    case WN:
      pred= NOTTAKEN;
      break;
    case WT:
      pred= TAKEN;
      break;
    case ST:
      pred= TAKEN;  
      break;
    default:
      pred = NOTTAKEN;
      printf("Warning: Invalid counter state!");
      break;  
  }
  return pred;
}

uint8_t two_bit_update(uint8_t state, uint8_t outcome) {
  if (outcome == TAKEN) {
    switch(state) {
      case SN: return WN;
      case WN: return WT;
      case WT: return ST;
      case ST: return ST;
      default: return state;
    }
  } else {
    switch(state) {
      case SN: return SN;
      case WN: return SN;
      case WT: return WN;
      case ST: return WT;
      default: return state;
    }
  }
}

uint8_t inc_3ctr(uint8_t ctr){
  switch(ctr){
    case NNN:
      return NNT;
      break;
    case NNT:
      return NTN;
      break;
    case NTN:
      return NTT;
      break;
    case NTT:
      return TNN;
      break;
    case TNN:
      return TNT;
      break; 
    case TNT:
      return TTN;
      break;  
    case TTN:
      return TTT;
      break;
    case TTT:
      return TTT;
      break;
    default:
      printf("Invalid counter state");
      return ctr;
      break;             
  }
}

uint8_t dec_3ctr(uint8_t ctr){
  switch(ctr){
    case NNN:
      return NNN;
      break;
    case NNT:
      return NNN;
      break;
    case NTN:
      return NNT;
      break;
    case NTT:
      return NTN;
      break;
    case TNN:
      return NTT;
      break; 
    case TNT:
      return TNN;
      break;  
    case TTN:
      return TNT;
      break;
    case TTT:
      return TTN;
      break;
    default:
      printf("Invalid counter state");
      return ctr;
      break;             
  }
}