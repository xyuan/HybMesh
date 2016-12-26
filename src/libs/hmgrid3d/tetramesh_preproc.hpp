#ifndef HMGRID3D_TETRAMESH_PREPROCESSOR_HPP
#define HMGRID3D_TETRAMESH_PREPROCESSOR_HPP

#include "surface_grid3d.hpp"
namespace HMGrid3D{namespace Mesher{

//splits surfaces by relatively smooth subareas,
//builds pyramids on all non-triangle surface cells.
//input tree should be correctly directed and have nesting level <=1
struct SurfacePreprocess{
	SurfacePreprocess(const HMGrid3D::SurfaceTree& tree, double split_angle);
	void Restore(HMGrid3D::GridData& g, const VertexData& gvert, const VertexData& svert);

	//======== data filled by constructor
	//data for splitted triangulated surface
	vector<vector<HMGrid3D::Surface>> decomposed_surfs;
	HMGrid3D::EdgeData ae;
	HMGrid3D::VertexData av;
	vector<vector<vector<HMGrid3D::EdgeData>>> decomposed_edges;

private:
	//constructor subroutings
	void supplement_from_decomposed_surfs();
	void assemble_bnd_grid();
	//data for splitted surface before triangulation
	vector<vector<HMGrid3D::Surface>> decomposed_surfs1;
	FaceData allfaces1;

	//extracted shallow subgrids from bndgrid
	//for each decomposed_edges entry
	vector<vector<GridData>> split_bnd_grid;
	GridData bnd_grid;

	void merge_with_bnd(GridData& tar, const vector<int>& tar_points, const vector<int>& bnd_points);
};



}}



#endif
