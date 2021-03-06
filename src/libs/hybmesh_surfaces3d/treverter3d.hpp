#ifndef HYBMESH_TREVERTER3D_HPP
#define HYBMESH_TREVERTER3D_HPP
#include "surface.hpp"
#include "surface_tree.hpp"

namespace HM3D{ namespace Surface{ namespace R{

//temporary reverts all face edges direction so that all
//face dirctions match each other.
//When this object dies it reverts faces back to its original state.
class Revert{
	FaceData* obj;
	vector<bool> need_revert;
	bool permanent;
public:
	//delete default constructors to avoid premature reversion.
	Revert(const Revert&) = delete;

	//using const since all surf changes are temporal
	Revert(const FaceData& srf);
	~Revert();

	void reverse_direction();
	void make_permanent(){permanent = true; }

	static void Permanent(FaceData& srf){
		Revert a(srf);
		a.make_permanent();
	}
};

//tree internal area is located to the left of even leveled surfaces
//and to the right of odd leveled surfaces.
//open surfaces direction is matched.
class RevertTree{
	Surface::Tree* obj;
	ShpVector<Revert> openrevs;
	ShpVector<Revert> closedrevs;
public:
	//delete default constructors to avoid premature reversion.
	RevertTree(const RevertTree&) = delete;

	//using const since all surf changes are temporal
	RevertTree(const Surface::Tree& srf);
	~RevertTree();

	void make_permanent(){
		for (auto& a: openrevs) a->make_permanent();
		for (auto& a: closedrevs) a->make_permanent();
	}

	static void Permenent(Surface::Tree& tree){
		RevertTree a(tree);
		a.make_permanent();
	}
};

//all faces wich have no left (cells_left=true) or right (cells_left=false)
//cell will be reverted
class RevertGridSurface{
	FaceData* obj;
	vector<bool> need_revert;
	bool permanent;
public:
	//delete default constructors to avoid premature reversion.
	RevertGridSurface(const RevertGridSurface&) = delete;

	//using const since all surf changes are temporal
	RevertGridSurface(const FaceData& srf, bool cells_left);
	~RevertGridSurface();

	void make_permanent(){ permanent = true; }

	static void Permanent(FaceData& srf, bool cells_left){
		RevertGridSurface a(srf, cells_left);
		a.make_permanent();
	}
};

}}}

#endif
