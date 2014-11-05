#include <boost/move/move.hpp>
#include "boost/multi_array.hpp"
#include "boost/cstdlib.hpp"
#include <array>
#include <iostream>

char cmat_[] = {
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
                              
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,
                              
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};
static const int cmat_h_ = 22;
static const int cmat_w_ = 14;

char cmat_O[] = {
    1,1,
    1,1
};
char cmat_T[] = {
    0, 1, 0,
    1, 1, 1,
    0, 0, 0
};
char cmat_Z[] = {
    1, 1, 0,
    0, 1, 1,
    0, 0, 0
};
char cmat_N[] = {
    0, 1, 1,
    1, 1, 0,
    0, 0, 0
};
char cmat_F[] = {
    1, 1, 0,
    0, 1, 0,
    0, 1, 0
};
char cmat_E[] = {
    0, 1, 1,
    0, 1, 0,
    0, 1, 0
};
char cmat_L[] = {
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0
};
typedef boost::multi_array<char,2> array2d;

template <typename T>
std::ostream& print2d(std::ostream& out, T const& m)
{
    return out;
    for (std::array<array2d::index,2> a = {{0,0}}; a[0] != m.size(); ++a[0]) {
        for (a[1] = 0; a[1] != m[0].size()-1; ++a[1])
            out << int( m(a) ) <<" ";
        out << int( m(a) ) <<"\n";
    }
    return out <<"\n";
}

template <typename M>
void rotate90_right(M& m)
{
    BOOST_ASSERT(m.size() == m[0].size());
    if (m.size() <= 1)
        return;

    {
        std::array<size_t,2> p4[4] = {
            {{0,0}}
            , {{0,m[0].size()-1}}
            , {{m.size()-1,m[0].size()-1}}
            , {{m.size()-1,0}}
        };
        for (array2d::index i = 0; i != m.size()-1; ++i) {
            for (int x=1; x < 4; ++x)
                std::swap(m(p4[0]), m(p4[x]));
            p4[0][1]++; p4[1][0]++; p4[2][1]--; p4[3][0]--;
        }
    }

    if (m.size() == 2)
        return;

    typedef array2d::index_range range;
    array2d::array_view<2>::type n = m[boost::indices
            [range(1,    m.size()-1)]
            [range(1, m[0].size()-1)]
        ];
    return rotate90_right(n);
}

// template <typename M, typename N>
// bool is_collision(M const& m, N const& n)
// {
//     BOOST_ASSERT(m.size() == n.size() && m[0].size() == n[0].size());
//     for (std::array<array2d::index,2> a = {{0,0}}; a[0] != m.size(); ++a[0]) {
//         for (a[1] = 0; a[1] != m[0].size(); ++a[1])
//             if (n(a) && m(a)) {
//                 //std::cout <<"#collision\n";
//                 print2d(std::cout, m);
//                 print2d(std::cout, n);
//                 //std::cout <<"/collision\n";
//                 return 1;
//             }
//     }
//     return 0;
// }
//
//template <typename M, typename N>
//M& or_assign(M& m, N const& n)
//{
//    BOOST_ASSERT(m.size() == n.size() && m[0].size() == n[0].size());
//    for (std::array<array2d::index,2> a = {{0,0}}; a[0] != m.size(); ++a[0]) {
//        for (a[1] = 0; a[1] != m[0].size(); ++a[1])
//            m(a) |= n(a);
//    }
//    return m;
//}

inline std::array<array2d::index,2> operator-(
        std::array<array2d::index,2> const& rhs, std::array<array2d::index,2> const& lhs)
{
    return std::array<array2d::index,2>{{rhs[0]-lhs[0], rhs[1]-lhs[1]}};
}

struct Shape2d
{
    size_t v2[2];
    Shape2d(array2d const& a) { v2[0]=a.shape()[0]; v2[1]=a.shape()[1]; }
    template <typename Array> Shape2d(Array const& a) { v2[0]=a[0]; v2[1]=a[1]; }
    friend std::ostream& operator<<(std::ostream& out, Shape2d const& d) {
        return out <<"<"<< d.v2[0]<<","<<d.v2[1] <<">";
    }
};

template <typename M, typename N>
inline std::array<array2d::index,2> common_ep(M const& m, std::array<array2d::index,2> const& bp, N const& n)
{
    return std::array<array2d::index,2>{{
        int(std::min(bp[0]+n.size(),m.size())), int(std::min(bp[1]+n[0].size(),m[0].size()))
    }};
}

template <typename M, typename N>
bool is_collision(M const& m, std::array<array2d::index,2> bp, N const& n)
{
    BOOST_ASSERT(bp[0]>=0 && bp[1]>=0);
    std::array<array2d::index,2> ep = common_ep(m,bp,n);//{{bp[0]+int(n.size()), bp[1]+int(n[0].size())}};
    // if (!(ep[0] <= m.size() && ep[1] <= m[0].size())) std::cerr << Shape2d(ep) << Shape2d(m) <<"\n";
    // BOOST_ASSERT(ep[0] <= m.size() && ep[1] <= m[0].size());

    for (auto p=bp; p[0] != ep[0]; ++p[0]) {
        for (p[1] = bp[1]; p[1] != ep[1]; ++p[1])
            if (n(p - bp) && m(p)) {
                //std::cout <<"#collision\n";
                //print2d(std::cout, m);
                //print2d(std::cout, n);
                //std::cout <<"/collision\n";
                return 1;
            }
    }
    return 0;
}

template <typename M, typename N>
M& or_assign(M& m, std::array<array2d::index,2> bp, N const& n)
{
    BOOST_ASSERT(bp[0]>=0 && bp[1]>=0);
    std::array<array2d::index,2> ep = common_ep(m,bp,n);//{{bp[0]+int(n.size()), int(bp[1]+n[0].size())}};
    BOOST_ASSERT(ep[0] <= int(m.size()) && ep[1] <= int(m[0].size()));
    for (auto p = bp; p[0] != ep[0]; ++p[0]) {
        for (p[1] = bp[1]; p[1] != ep[1]; ++p[1])
        {
            m(p) |= n(p - bp);
        }
    }
    return m;
}

struct Main // : array2d
{
    std::vector<std::pair<char*,size_t>> const const_mats_;
    array2d mat_, smat_, pv_;
    std::array<array2d::index,2> p_;
    time_t tb_, td_;

    Main() // (char v[], size_t N, size_t n)
        //: array2d(boost::extents[N/n][n])
        : const_mats_(mats_init())
    {
        // BOOST_ASSERT(N % n == 0); assign(v, N);
        ::srand(time(0));
    }

    void start()
    {
        BOOST_ASSERT(cmat_h_ * cmat_w_ == sizeof(cmat_));
        mat_.resize(boost::extents[cmat_h_][cmat_w_]);
        mat_.assign(cmat_, cmat_ + sizeof(cmat_));
        tb_ = time(0);
        print2d(std::cout, mat_); // std::cout<<p_[0]<<p_[1]<<"\n" <<z[0]<<z[1]<<"\n";

        take_pv(0);
        next_round();
    }

    bool Move(int di)
    {
        //std::cout <<p_[0]<<p_[1]<< "\n";
        auto p = p_;
        if (di == 0) {
            p[0]++;
        } else if (di < 0) {
            p[1]--;
        } else {
            p[1]++;
        }

        //typedef array2d::index_range range;
        //array2d::array_view<2>::type av = mat_[boost::indices
        //        [range(p[0],    smat_.size()+p[0])]
        //        [range(p[1], smat_[0].size()+p[1])]
        //    ];
        
        if (is_collision(mat_, p, smat_)) {
            if (di == 0) {
                or_assign(mat_, p_, smat_);
                collapse(p_[0]+(smat_.size()-1), p_[0]);
            }
            return false;
        }

        p_ = p;
        //std::cout <<p_[0]<<p_[1]<< "\n";
        if (di == 0) {
            td_ = time(0);
        }
        return true;
    }

    bool next_round()
    {
        take_pv(&smat_);
        p_[1] = int(mat_[0].size() - smat_[0].size()) / 2;
        p_[0] = -int(smat_.size()-1);
        while (p_[0] <= 0) {
            typedef array2d::index_range range;
            //size_t const* z = smat_.shape();
            //array2d::array_view<2>::type aw = mat_[boost::indices
            //        [range(0,    smat_.size()+p_[0])]
            //        [range(p_[1], smat_[0].size()+p_[1])]
            //    ];
            //print2d(std::cout, aw);
            array2d::array_view<2>::type av = smat_[boost::indices
                    [range(-p_[0],    smat_.size())]
                    [range(0, smat_[0].size())]
                ];
            print2d(std::cout, av);
            if (is_collision(mat_, {{0,p_[1]}}, av)) {
                over();
                return 0;
            }

            if (p_[0] == 0)
                break;
            ++p_[0];
        } // while (++p_[0] != 0);
        td_ = time(0);
        return 1;
    }

    bool rotate()
    {
        auto tmp = smat_;
        rotate90_right(tmp);
        if (is_collision(mat_, p_, tmp)) {
            return false;
        }
        std::swap(smat_,tmp);
        return true;
    }

private:
    void over()
    {
        std::cout << "over\n";
    }

    void clear(array2d::index row) {
        for (auto& x : mat_[row])
            x = 0;
    }
    template <typename R> void move_(array2d::index src, array2d::index dst) {
        mat_[dst] = mat_[src];
        clear(src);
    }

    void collapse(array2d::index rb, array2d::index r0, int nc=0)
    {
        auto const & row = this->mat_[rb];

        if (rb <= r0) {
            if (std::find(row.begin(), row.end(), 0) == row.end()) {
                clear(rb);
                ++nc;
            } else if (nc > 0) {
                mat_[rb+nc] = row; clear(rb);
            } else if (rb == r0 && nc == 0) {
                return;
            }
        } else if (nc > 0) {
            // auto pred = [](int x) -> bool { return x!=0; };
            // if (std::find(row.begin(), row.end(), pred) == row.end())
                ;
            mat_[rb+nc] = row; clear(rb);
        }

        if (rb > 0) {
            collapse(rb-1, r0, nc);
        }
    }

    void take_pv(array2d* a)
    {
        if (a) {
            a->resize(boost::extents[pv_.size()][pv_[0].size()]);
            *a = pv_;
            print2d(std::cout, *a);
        }
        auto p = const_mats_[::rand() % const_mats_.size()];
        int x = ::sqrt(p.second);
        pv_.resize(boost::extents[x][x]);
        pv_.assign(p.first, p.first + p.second);
        print2d(std::cout, pv_);
    }

    static std::vector<std::pair<char*,size_t>> mats_init()
    {
        std::vector<std::pair<char*,size_t>> mats;
        mats.emplace_back(cmat_O, sizeof(cmat_O));
        mats.emplace_back(cmat_T, sizeof(cmat_T));
        mats.emplace_back(cmat_Z, sizeof(cmat_Z));
        mats.emplace_back(cmat_N, sizeof(cmat_N));
        mats.emplace_back(cmat_F, sizeof(cmat_F));
        mats.emplace_back(cmat_E, sizeof(cmat_E));
        mats.emplace_back(cmat_L, sizeof(cmat_L));
        return boost::move(mats);
    }

    friend std::ostream& operator<<(std::ostream& out, Main const& M)
    {
        auto& m = M.mat_;
        for (array2d::index i = 0; i != m.size(); ++i)
        {
            for (array2d::index j = 0; j != m[0].size()-1; ++j)
            {
                if (i >= M.p_[0] && i < M.p_[0]+M.smat_.size()
                        && (j >= M.p_[1] && j < M.p_[1]+M.smat_[0].size())) {
                    std::cout << int(m[i][j] || M.smat_[i-M.p_[0]][j-M.p_[1]]) <<" ";
                    continue;
                }
                std::cout << int(m[i][j]) <<" ";
            }
            std::cout << int(m[i][m[0].size()-1])<<"\n";
        }
        return out;
    }
};

int main(int argc, char* const argv[])
{
    Main M;
    M.start();
    std::cout << M << "\n";
    M.rotate();
    std::cout << M << "\n";
    while (M.Move(-1))
        ; std::cout << M << "\n";
    while (M.Move(1))
        ; std::cout << M << "\n";
    while (M.Move(0))
        ; std::cout << M << "\n";
    //std::cout << M << "\n";
    return 0;
}


int main_x()
{
    array2d mat(boost::extents[21][12]);
    mat.assign(cmat_,cmat_+sizeof(cmat_));

    array2d sa(boost::extents[3][3]);
    sa.assign(cmat_L,cmat_L+sizeof(cmat_L));

    typedef array2d::index_range range;

    int x = 0, y = 3;
    array2d::array_view<2>::type vm = mat[boost::indices
            [range(y,    sa.size()+y)]
            [range(x, sa[0].size()+x)]
        ];

    std::cout << sa.size() <<" "<< sa[0].size() << "\n";
    std::cout << vm.size() <<" "<< vm[0].size() << "\n";

    for (array2d::index i = 0; i != sa.size(); ++i)
    {
        for (array2d::index j = 0; j != sa[0].size(); ++j)
            if (vm[i][j] && sa[i][j])
                ;
        std::cout <<"\n"<< int(vm[i][0] && sa[i][0]);
        for (array2d::index j = 1; j != sa[0].size(); ++j)
        {
            std::cout <<" "<< int(vm[i][j] && sa[i][j]);
        }
    }
    std::cout <<"\n";

    return boost::exit_success;
}

