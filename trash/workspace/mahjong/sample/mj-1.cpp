#include <math.h>
#include <stdio.h>
#include <memory>
#include <array>
#include <algorithm>
#include <boost/spirit/include/qi.hpp>

template <typename... A> void err_exit_(int lin_, char const* fmt, A... a) {
    fprintf(stderr, fmt, lin_, a...); exit(127);
}
template <typename... A> void err_msg_(int lin_, char const* fmt, A... a) {
    fprintf(stderr, fmt, lin_, a...);
}
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)

// 万 筒 索 字
// 东 南 西 北 中 发 白
// 顺 刻 将
struct mahjong : std::array<std::array<int8_t,9>,4>
{
    std::array<int8_t,4> count;
    int8_t jindex = -1;

    static char* parse(std::array<int8_t,9>& mj, char const* beg, char const* end);
    static bool parse_file(mahjong& mj, mahjong& mj, FILE* fp);
};

bool mahjong::parse_file(mahjong& mj, FILE* fp)
{
    //std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
    char buf[1024*4];
    int x = 0;
    while (x < 4) {
        if (!fgets(buf, sizeof(buf), fp)) {
            break;
        }
        char* pos = buf;
        char* end = &buf[strlen(buf)];
        do {
            char* pp = pos;
            if ( (pos = parse(mj[x], pos, end))) {
                mj.count[x] = std::accumulate(mj[x].begin(), mj[x].end(), 0);
                ++x;
            } else {
                ERR_MSG("parse: %s", pp);
                return false;
            }
        } while (pos != end);
    }
    return (x == 4);
}

char* mahjong::parse(std::array<int8_t,9>& a, char const* pos, char const* end)
{
    std::vector<int> v;
    if (qi::phrase_parse(pos,end, *qi::int_ >> (';'|qi::eoi), qi::space, v)) {
        if (v.size() > 9)
            return 0;
        v.resize(9);
        std::copy(v.begin(),v.end(), a.begin());
    }
    return const_cast<char*>(pos);
}

//???
bool Win(int [4][10]);
bool Analyze(int [],bool);

int main(int argc, char* argv[])
{
   //?
   int allPai[4][10]={
                     {6,1,4,1},//
                     {3,1,1,1},//
                     {0},//?
                     {5,2,3}//
                     };
   if (Win (allPai))
       printf("Hu!\n");
   else
       printf("Not Hu!\n");
   return 0;
}

//???????
bool Win (int allPai[4][10])
{
   int jiangPos;//""?
   int yuShu;//?
   bool jiangExisted=false;
   //????3,3,3,3,2?
   for(int i=0;i<4;i++)
   {
       yuShu=(int)fmod(allPai[i][0],3);
       if (yuShu==1)
       {
            return false;
       }
       if (yuShu==2) {
            if (jiangExisted)
            {
                 return false;
            }
            jiangPos=i;
            jiangExisted=true;
       }
   }
   for(int i=0;i<4;i++)
   {
       if (i!=jiangPos) {
            if (!Analyze(allPai[i],i==3))
            {
                 return false;
            }
       }
   }
   //????,?????,,??
   bool success=false;//??""??
   for(int j=1;j<10;j++)//?,?j??
   {
       if (allPai[jiangPos][j]>=2)
       {
            //??2?
            allPai[jiangPos][j]-=2;
            allPai[jiangPos][0]-=2;
            if(Analyze(allPai[jiangPos],jiangPos==3))
            {
                 success=true;
            }
            //2?
            allPai[jiangPos][j]+=2;
            allPai[jiangPos][0]+=2;
            if (success) break;
       }
   }
   return success;
}

//?"?""?"
bool Analyze(int aKindPai[],bool ziPai)
{
   if (aKindPai[0]==0)
   {
       return true;
   }
//????
   int j;
   for(j=1;j<10;j++)
   {
       if (aKindPai[j]!=0)
        {
            break;
       }
   }
   bool result;
   if (aKindPai[j]>=3)//??
   {
       //??3??
       aKindPai[j]-=3;
       aKindPai[0]-=3;
       result=Analyze(aKindPai,ziPai);
       //3??
       aKindPai[j]+=3;
       aKindPai[0]+=3;
       return result;
   }
   //??
   if ((!ziPai)&&(j<8)
       &&(aKindPai[j+1]>0)
       &&(aKindPai[j+2]>0))
   {
       //??3??
       aKindPai[j]--;
       aKindPai[j+1]--;
       aKindPai[j+2]--;
       aKindPai[0]-=3;
       result=Analyze(aKindPai,ziPai);
       //3??
       aKindPai[j]++;
       aKindPai[j+1]++;
       aKindPai[j+2]++;
       aKindPai[0]+=3;
       return result;
   }
   return false;
}

