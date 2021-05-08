#include <iostream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <cstring>
#include <string>
using BYTE = uint8_t;
using SBYTE = int8_t;
#define READ_BUF_SIZ 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define PHYS_MEM 65536
#define PAGE_TABLE 256
#define TLB_SIZE 32
#define SIMULATION 500 
using namespace std;

class File{
private:
    FILE* fp;
public:
    File() = default;

    //define default constructor
    File(const char* fname){
        this->fp = fopen(fname, "rec");
    }
    ~File(){
        fclose(this->fp);
    }

    //operator (overload)
    char operator[](int index) const{
        fseek(this->fp, index, 0);
        char ret = fgetc(this->fp);
        return ret;
    }
    
    
    int parse(){
        char line[READ_BUF_SIZ];
        memset(line, 0, sizeof(line));
        fgets(line, READ_BUF_SIZ, fp);

        int index = READ_BUF_SIZ - 1;
        for(; index >= 0; --index){
            if(line[index] == ':'){
                ++index; break;
            }
        }
        return stoi((char*)&line[index]); 
    }

    int getAdd(){
        char line[READ_BUF_SIZ];
        fgets(line, READ_BUF_SIZ, fp);
        return stoi(line);
    }
};


class LRU_tlb{
private:
    queue<int> least_Recent_Cache;
    unordered_map<int, int> tlb;
    using HashItr = unordered_map<int, int>::iterator;
    int tlb_h, tlb_m;

public:
    LRU_tlb() : tlb_h(0), tlb_m(0) { }

    //find if we have page number store in tlb
    bool cont(int page_Num){
        auto f = tlb.find(page_Num);
        if(f == tlb.end()) {
            ++tlb_m;
             return false; 
        }
        ++tlb_h;
        return true;
    }

    void push(int page_Num, int phys_addy){
        if(tlb.size() > TLB_SIZE){
            auto least_Recently_Used = least_Recent_Cache.front();
            least_Recent_Cache.pop();
            tlb.erase(least_Recently_Used);
        }
        tlb[page_Num] = phys_addy;
        least_Recent_Cache.push(page_Num);
    }
    decltype(tlb)::mapped_type operator[](int page_number){
        return tlb[page_number];
    }
    decltype(tlb_m) getMisses(){ return this->tlb_m;}
    decltype(tlb_h)   getHits()  { return this->tlb_h;}
    
};

class LogAdd{
public:
    LogAdd(int _address){
        this->add = static_cast<decltype(this->add)>(_address);
    }

    BYTE getPage(){
        //shift operator move to right 8 bits 
        return (0xFF00 & this->add) >> 8;
    }
    BYTE getOffset(){
        return (0xFF & this->add);
    }

private:
    uint16_t add; 
};

int main(int argc, const char* argv[]){

    class Mng{
    File* address, *correct, *binary;
    LRU_tlb tlb;
public:
    //default constructor
    Mng() : current_frame(0), page_index(0),
                        page_Fault(0), tot_pages(0) {
        address   = new File("addresses.txt");
        correct = new File("correct.txt");
        binary  = new File("BACKING_STORE.bin");
        memset(page_table, -1, sizeof(page_table));
    }
    //destructor
    ~Mng(){
        delete address;
        delete correct;
        delete binary;
    }
    private:
    SBYTE physical_mem[PHYS_MEM];
    int page_Fault;
    int tot_pages;
    int current_frame;
    int page_table[PAGE_TABLE]; 
    int page_index;
    // retrieve value of physical memory 
    SBYTE getValue(int address){
        LogAdd la(address);
        /* get the page and the offset */
        BYTE page_number = la.getPage(), 
             offset      = la.getOffset();

        int physical_addy;

        
        bool tlb_hit = tlb.cont(page_number);

        if(tlb_hit){ 
            printf("tlb accept!\n");
            physical_addy = tlb[page_number] + offset;
        }else{ 
            printf("tlb missing!\n");
            physical_addy = page_table[page_number];
            if(physical_addy == -1){
                physical_addy = pageFaultHandeler(page_number);
            } ++tot_pages;
            tlb.push(page_number, physical_addy);
            physical_addy += offset;
        }

        printf("printing virtual address at: %d Physical address at: %d Value: %d ", 
                    address, physical_addy, physical_mem[physical_addy]);
        return physical_mem[physical_addy];
    }

    //check page fault
    int pageFaultHandeler(BYTE page_Number){
        int pz = PAGE_SIZE;
        int current_adddress = current_frame*FRAME_SIZE;
        printf("PAGE FAULT AT: %d\n", page_Number);
        ++page_Fault;

        for(int x{}; x < pz; ++x){
            physical_mem[current_adddress + x] = (*binary)[page_Number*pz + x];
        }

        page_table[page_Number] = current_adddress;

        current_frame = (current_frame + 1) % FRAME_SIZE;
        return current_adddress;
    }
    //test address
    void test(int address){
        LogAdd la(address);
        printf("page at: %d, offset at: %d\n", la.getPage(), la.getOffset());
    }

    void simulate(){
        int tot{}, tot_correct{};
        //read addresses in order to compare 
        int value, cvalue;
        for(int x{}; x < SIMULATION; ++x, ++tot){
            value = this->getValue(address->getAdd());
            cvalue = correct->parse();
            printf("correct calue: %d\n", cvalue);
            if(value == cvalue) ++tot_correct;
        }
        float accuracy = (float)tot_correct/(float)tot;
        printf("----Accuracy: %.0f%%----", accuracy*100.0); 
        auto hits = tlb.getHits(), 
        misses = tlb.getMisses();
        printf("we are goint to orint hits and misses ratio\n");
        printf("tlb hits at: %d----tlb misses: %d----tlb_hits_ratio: %.0f%%\n", hits, misses, (float(hits)/float(hits + misses))*100);
        printf("page fault ratio: %0.f%%", (float)page_Fault/(float)tot_pages * 100);
    }
};

    return 0;
}

