#ifndef HYBMESH_HMFEM_FEMMAT_HPP
#define HYBMESH_HMFEM_FEMMAT_HPP
#include "hmproject.h"
#include "femgrid43.hpp"

namespace HMFem{ namespace Impl{

//Sparce Matrix
struct Mat{
private:
	vector<std::map<int, double>> data;
public:
	//get/set methods
	int rows() const {return data.size();}
	void clear_row(int i) { data[i].clear(); }
	void set(int i, int j, double val);
	double get(int i, int j) const;
	void add(int i, int j, double val);
	vector<double> diag() const;

	//Methods
	double RowMultVec(const vector<double>& u, int irow) const;
};


//dense matrix of little dimension
struct LocMat{
	virtual void ToMat(const vector<int>& pind, Mat& target) const = 0;
};

struct LocMat3Sym: public LocMat, public std::array<double, 6>{
	void ToMat(const vector<int>& pind, Mat& target) const;
};

struct LocMat4Sym: public LocMat, public std::array<double, 10>{
	void ToMat(const vector<int>& pind, Mat& target) const;
};

//Solving procedures
class MatSolve{
	Mat* m;
public:
	MatSolve(){};
	void Init(Mat& mat);
	void Solve(const vector<double>& rhs, vector<double>& x);
};



}}//Impl
#endif
