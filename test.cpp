#include "fst/fstapi.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct FstVar
{
    std::string name;
    bool is_alias;
    std::string scope;
    std::vector<FstVar> aliases;
};

class FstData
{
  public:
    FstData(std::string filename);
    ~FstData();

    uint64_t getStartTime();
    uint64_t getEndTime();

    void extractVarNames();

    void callback(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen);
  private:
    struct fstReaderContext *ctx;
    std::vector<std::string> scopes;
    std::map<fstHandle, FstVar> handle_to_var;
};

FstData::FstData(std::string filename) : ctx(nullptr)
{
    ctx = (fstReaderContext *)fstReaderOpen(filename.c_str());
    printf("%zu\n", fstReaderGetScopeCount(ctx));
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

void FstData::callback(uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    FstVar var = handle_to_var[pnt_facidx];
    printf("%zu %s = %s\n", pnt_time, var.name.c_str(), pnt_value);
}


static void value_change_callback_varlen(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
    FstData *ptr = (FstData*)user_data;
    ptr->callback(pnt_time, pnt_facidx, pnt_value, plen);
}

static void value_change_callback(void *user_data, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value)
{
    FstData *ptr = (FstData*)user_data;
    uint32_t plen = (pnt_value) ?  strlen((const char *)pnt_value) : 0;
    ptr->callback(pnt_time, pnt_facidx, pnt_value, plen);
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
            var.is_alias = h->u.var.is_alias;
            var.name = remove_spaces(h->u.var.name);
            var.scope = scopes.back();
            if (!var.is_alias)
                handle_to_var[h->u.var.handle] = var;
            else
                handle_to_var[h->u.var.handle].aliases.push_back(var);
            break;
        }
        }
    }
    for (auto &val : handle_to_var) {
        printf("var : %s.%s %d\n", val.second.scope.c_str(), val.second.name.c_str(), (int)val.second.aliases.size());
    }

    fstReaderSetFacProcessMaskAll(ctx);
    fstReaderIterBlocks2(ctx, value_change_callback, value_change_callback_varlen, this, nullptr);
}

/*
int             fstReaderGetFacProcessMask(void *ctx, fstHandle facidx);
int             fstReaderGetFileType(void *ctx);
int             fstReaderGetFseekFailed(void *ctx);
fstHandle       fstReaderGetMaxHandle(void *ctx);
uint64_t        fstReaderGetMemoryUsedByWriter(void *ctx);
uint32_t        fstReaderGetNumberDumpActivityChanges(void *ctx);
uint64_t        fstReaderGetScopeCount(void *ctx);
*/
int main(int argc, char **argv)
{

    FstData f("grom_computer_tb.fst");
    printf("Start %zu end %zu\n", f.getStartTime(), f.getEndTime());
    f.extractVarNames();

    // fstReaderSetFacProcessMaskAll(xc);		/* these 3 lines do all the VCD
    // writing work */ fstReaderIterBlocks(xc, NULL, NULL, fv);	/* these 3 lines
    // do all the VCD writing work */
}
