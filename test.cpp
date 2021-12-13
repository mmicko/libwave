#include "fst/fstapi.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct FstVar
{
    fstHandle id;
    std::string name;
    bool is_alias;
    std::string scope;
    int width;
};

class FstData
{
  public:
    FstData(std::string filename);
    ~FstData();

    uint64_t getStartTime();
    uint64_t getEndTime();

    std::vector<FstVar>& getVars() { return vars; };

    void reconstruct_callback(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen);
    void reconstruct(std::vector<fstHandle> &signal);
    void reconstuctAll();

    void reconstruct_callback_attimes(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen);
    void reconstructAtTimes(std::vector<fstHandle> &signal,std::vector<uint64_t> time);

    std::string valueAt(fstHandle signal, uint64_t time);
    std::vector<uint64_t> edges(fstHandle signal, bool positive, bool negative);
    void recalc_time_offsets(fstHandle signal, std::vector<uint64_t> time);
  private:
    void extractVarNames();

    struct fstReaderContext *ctx;
    std::vector<std::string> scopes;
    std::vector<FstVar> vars;
    std::map<fstHandle, FstVar> handle_to_var;
    std::map<fstHandle, std::vector<std::pair<uint64_t, std::string>>> handle_to_data;
    std::map<fstHandle, std::map<uint64_t, size_t>> time_to_index;
    std::map<fstHandle, std::map<size_t, uint64_t>> index_to_time;
    std::vector<uint64_t> sample_times;
    size_t sample_times_ndx;
    std::map<fstHandle, std::string> current;
};

FstData::FstData(std::string filename) : ctx(nullptr)
{
    ctx = (fstReaderContext *)fstReaderOpen(filename.c_str());
    extractVarNames();
}

FstData::~FstData()
{
    if (ctx)
        fstReaderClose(ctx);
}

uint64_t FstData::getStartTime() { return fstReaderGetStartTime(ctx); }

uint64_t FstData::getEndTime() { return fstReaderGetEndTime(ctx); }

static std::string remove_spaces(std::string str)
{
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    return str;
}

void FstData::extractVarNames()
{
    struct fstHier *h;
    intptr_t snum = 0;

    while (h = fstReaderIterateHier(ctx)) {
        switch (h->htyp) {
            case FST_HT_SCOPE: {
                snum++;
                std::string fst_scope_name = fstReaderPushScope(ctx, h->u.scope.name, (void *)(snum));
                scopes.push_back(fst_scope_name);
                break;
            }
            case FST_HT_UPSCOPE: {
                fstReaderPopScope(ctx);
                snum = fstReaderGetCurrentScopeLen(ctx) ? (intptr_t)fstReaderGetCurrentScopeUserInfo(ctx) : 0;
                break;
            }
            case FST_HT_VAR: {
                FstVar var;
                var.id = h->u.var.handle;
                var.is_alias = h->u.var.is_alias;
                var.name = remove_spaces(h->u.var.name);
                var.scope = scopes.back();
                var.width = h->u.var.length;
                vars.push_back(var);
                if (!var.is_alias)
                    handle_to_var[h->u.var.handle] = var;
                break;
            }
        }
    }
}

static void reconstruct_clb_varlen(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    FstData *ptr = (FstData*)user_data;
    ptr->reconstruct_callback(pnt_time, pnt_facidx, pnt_value, plen);
}

static void reconstruct_clb(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value)
{
    FstData *ptr = (FstData*)user_data;
    uint32_t plen = (pnt_value) ?  strlen((const char *)pnt_value) : 0;
    ptr->reconstruct_callback(pnt_time, pnt_facidx, pnt_value, plen);
}

void FstData::reconstruct_callback(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    handle_to_data[pnt_facidx].push_back(std::make_pair(pnt_time, std::string((const char *)pnt_value)));
    size_t index = handle_to_data[pnt_facidx].size() - 1;
    time_to_index[pnt_facidx][pnt_time] = index;
    index_to_time[pnt_facidx][index] = pnt_time;
}

void FstData::reconstruct(std::vector<fstHandle> &signal)
{
    handle_to_data.clear();
    time_to_index.clear();
    index_to_time.clear();
    fstReaderClrFacProcessMaskAll(ctx);
    for(const auto sig : signal)
        fstReaderSetFacProcessMask(ctx,sig);
    fstReaderIterBlocks2(ctx, reconstruct_clb, reconstruct_clb_varlen, this, nullptr);
}

void FstData::reconstuctAll()
{
    handle_to_data.clear();
    time_to_index.clear();
    index_to_time.clear();
    fstReaderSetFacProcessMaskAll(ctx);
    fstReaderIterBlocks2(ctx, reconstruct_clb, reconstruct_clb_varlen, this, nullptr);
}

static void reconstruct_clb_varlen_attimes(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    FstData *ptr = (FstData*)user_data;
    ptr->reconstruct_callback_attimes(pnt_time, pnt_facidx, pnt_value, plen);
}

static void reconstruct_clb_attimes(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value)
{
    FstData *ptr = (FstData*)user_data;
    uint32_t plen = (pnt_value) ?  strlen((const char *)pnt_value) : 0;
    ptr->reconstruct_callback_attimes(pnt_time, pnt_facidx, pnt_value, plen);
}

void FstData::reconstruct_callback_attimes(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    if (sample_times_ndx > sample_times.size()) return;
    uint64_t time = sample_times[sample_times_ndx];
    // if we are past the timestamp
    if (pnt_time > time) {
        for (auto const& c : current)
        {
            handle_to_data[c.first].push_back(std::make_pair(time,c.second));
            size_t index = handle_to_data[c.first].size() - 1;
            time_to_index[c.first][time] = index;
            index_to_time[c.first][index] = time;
        }
        sample_times_ndx++;
    }
    // always update current
    current[pnt_facidx] =  std::string((const char *)pnt_value);
}

void FstData::reconstructAtTimes(std::vector<fstHandle> &signal, std::vector<uint64_t> time)
{
    handle_to_data.clear();
    time_to_index.clear();
    index_to_time.clear();
    current.clear();
    sample_times_ndx = 0;
    sample_times = time;
    fstReaderClrFacProcessMaskAll(ctx);
    for(const auto sig : signal)
        fstReaderSetFacProcessMask(ctx,sig);
    fstReaderIterBlocks2(ctx, reconstruct_clb_attimes, reconstruct_clb_varlen_attimes, this, nullptr);

    if (time_to_index[signal.back()].count(time.back())==0) {
        for (auto const& c : current)
        {
            handle_to_data[c.first].push_back(std::make_pair(time.back(),c.second));
            size_t index = handle_to_data[c.first].size() - 1;
            time_to_index[c.first][time.back()] = index;
            index_to_time[c.first][index] = time.back();
        }
    }
}

std::string FstData::valueAt(fstHandle signal, uint64_t time)
{
    // TODO: Check if signal exist
    auto &data = handle_to_data[signal];
    if (time_to_index[signal].count(time)!=0) {
        size_t index = time_to_index[signal][time];
        return data.at(index).second;
    } else {
        size_t index = 0;
        for(size_t i = 0; i< data.size(); i++) {
            uint64_t t = index_to_time[signal][i];
            if (t > time) 
                break;
            index = i;
        }
        return data.at(index).second;
    }
}

std::vector<uint64_t> FstData::edges(fstHandle signal, bool positive, bool negative)
{
    // TODO: Check if signal exist
    auto &data = handle_to_data[signal];
    std::string prev = "x";
    std::vector<uint64_t> retVal;
    for(auto &d : data) {
        if (positive && prev=="0" && d.second=="1")
            retVal.push_back(d.first);
        if (negative && prev=="1" && d.second=="0")
            retVal.push_back(d.first);
        prev = d.second;
    }
    return retVal;
}

void FstData::recalc_time_offsets(fstHandle signal, std::vector<uint64_t> time)
{
    size_t index = 0;
    auto &data = handle_to_data[signal];
    for(auto curr : time) {
        for(size_t i = index; i< data.size(); i++) {
            uint64_t t = index_to_time[signal][i];
            if (t > curr) 
                break;
            index = i;
        }
        time_to_index[signal][curr] = index;
    }
}

int main(int argc, char **argv)
{

    FstData f("grom_computer_tb.fst");
    printf("Start %zu end %zu\n", f.getStartTime(), f.getEndTime());
    for (auto &val : f.getVars()) {
        printf("%zu var : %s.%s %d  %d\n", val.id, val.scope.c_str(), val.name.c_str(), (int)val.is_alias, val.width);
    }
    f.reconstuctAll();
    //printf("%s \n",f.valueAt(9,605000).c_str());
    auto v = f.edges(3,true, true);
    /*for(auto &val : v) {
        printf("%zu\n",val);
    }*/
    std::vector<fstHandle> signal;
    signal.push_back(9);
    signal.push_back(4);
    signal.push_back(5);
    signal.push_back(3); // clock must be in list
    f.reconstructAtTimes(signal,v);
}
