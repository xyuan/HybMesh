#include <sstream>
#include <fstream>
#include "addalgo.hpp"
#include "export3d_vtk.hpp"
#include "serialize3d.hpp"

using namespace HM3D;
namespace hme = HM3D::Export;

HMCallback::FunctionWithCallback<hme::TGridVTK> hme::GridVTK;
HMCallback::FunctionWithCallback<hme::TBoundaryVTK> hme::BoundaryVTK;
HMCallback::FunctionWithCallback<hme::TAllVTK> hme::AllVTK;
HMCallback::FunctionWithCallback<hme::TSurfaceVTK> hme::SurfaceVTK;

//==================== cell expression implementation
int hme::vtkcell_expression::wsize() const { return 1+pts.size(); }
std::string hme::vtkcell_expression::to_string() const {
	std::ostringstream out;
	out<<pts.size();
	for (auto i:pts) out<<" "<<i;
	return out.str();
}
std::string hme::vtkcell_expression::stype() const { return std::to_string(celltype); }

bool hme::vtkcell_expression::_false_return(){
	pts.clear();
	celltype = 0;
	return false;
}

int hme::vtkcell_expression::find_opposite(std::vector<std::vector<int>>& data, int cind, int pind){
	//find point opposite to data[cind][pind]
	int p_oppose = -1;
	for (int i=0; i<data.size(); ++i) if (i!=cind){
		auto fnd = std::find(data[i].begin(), data[i].end(), data[cind][pind]);
		if (fnd == data[i].end()) continue;
		auto fndnext = std::next(fnd);
		if (fndnext == data[i].end()) fndnext = data[i].begin();
		if (std::find(data[cind].begin(), data[cind].end(), *fndnext) ==
				data[cind].end()){
			p_oppose = *fndnext;
			break;
		}
		auto fndprev = (fnd == data[i].begin()) ? --data[i].end() : std::prev(fnd);
		if (std::find(data[cind].begin(), data[cind].end(), *fndprev) ==
				data[cind].end()){
			p_oppose = *fndprev;
			break;
		}
	}
	return p_oppose;
}
int hme::vtkcell_expression::is_opposite(std::vector<std::vector<int>>& data, int i1, int i2){
	//returns -1 or index of data[i2] which matches data[i1][0]
	int p_oppose = find_opposite(data, i1, 0);
	if (p_oppose < 0) return -1;
	auto fnd = std::find(data[i2].begin(), data[i2].end(), p_oppose);
	if (fnd != data[i2].end()) return fnd - data[i2].begin();
	else return -1;
}
std::pair<int, int> hme::vtkcell_expression::get_opposite(std::vector<std::vector<int>>& data, int ic){
	// returns index of opposite face, number of edge within data[first]
	// array which mathces data[ic][0] vertex. (-1, -1) if no oppsite was found
	std::pair<int, int> bad_ret(-1, -1);
	//find vertex opposite to data[ic][0] as one that is connected to data[ic][0]
	//but not lieing in data[ic]
	int p_oppose = find_opposite(data, ic, 0);
	if (p_oppose<0) return bad_ret;
	//find opposite face as face not containing any data[ic] nodes
	//but containing p_oppose
	for (auto i=0; i<data.size(); ++i) if (i != ic){
		//same number of points
		if (data[i].size() != data[ic].size()) continue;
		//no dublicate points
		std::unordered_set<int> dub(data[i].begin(), data[i].end());
		dub.insert(data[ic].begin(), data[ic].end());
		if (dub.size() != data[i].size() + data[ic].size()) continue;
		//find p_oppose
		auto fnd = std::find(data[i].begin(), data[i].end(), p_oppose);
		if (fnd == data[i].end()) continue;
		return std::make_pair(i, fnd-data[i].begin());
	}
	return bad_ret;
}
bool hme::vtkcell_expression::try_tetrahedron(std::vector<std::vector<int>>& data){
	celltype = 10;
	if (data.size() != 4) return _false_return();
	for (auto& d: data) if (d.size() != 3) return _false_return();
	//for (auto& d: data) pts.insert(pts.end(), d.begin(), d.end()); 
	//pts = aa::no_dublicates(pts);
	//if (pts.size() == 4) return true;
	//else return _false_return();
	pts = {data[0][0], data[0][2], data[0][1], -1};
	for (int i=0; i<3; ++i){
		int p3 = data[1][i];
		if (p3!=pts[0] && p3!=pts[1] && p3!=pts[2]){
			pts[3] = p3;
			break;
		}
	}
	if (pts.back() == -1) return _false_return();
	else return true;
}
bool hme::vtkcell_expression::try_hexahedron(std::vector<std::vector<int>>& data){
	celltype = 12;
	if (data.size() != 6) return _false_return();
	for (auto& d: data) if (d.size() != 4) return _false_return();
	//find opposite face
	std::pair<int, int> opcell = get_opposite(data, 0);
	if (opcell.first < 0) return _false_return();
	std::vector<int> dlower = data[0];
	std::vector<int> dupper = data[opcell.first];
	std::rotate(dupper.begin(), dupper.begin()+opcell.second, dupper.end());
	pts = {dlower[0], dlower[3], dlower[2], dlower[1],
		dupper[0], dupper[1], dupper[2], dupper[3]};
	return true;
}
bool hme::vtkcell_expression::try_pyramid(std::vector<std::vector<int>>& data){
	celltype = 14;
	if (data.size() != 5) return _false_return();
	int f4 = -1;
	for (int i=0; i<data.size(); ++i){
		if (data[i].size() == 4){ f4 = i; break; }
		if (data[i].size() != 3) return _false_return();
	}
	if (f4 == -1) return _false_return();
	int cp = -1;
	int itry = (f4 == 0)?1:0;
	for (int j=0; j<data[itry].size(); ++j){
		auto fnd = std::find(data[f4].begin(), data[f4].end(), data[itry][j]);
		if (fnd == data[f4].end()){
			cp = data[itry][j];
			break;
		}
	}
	if (cp == -1) return _false_return();
	pts = {data[f4][0], data[f4][3], data[f4][2], data[f4][1], cp};
	return true;
}
bool hme::vtkcell_expression::try_wedge(std::vector<std::vector<int>>& data){
	celltype=13; 
	if (data.size() != 5) return _false_return();
	int f1=-1, f2=-1;
	for (int i=0; i<data.size(); ++i){
		auto& c = data[i];
		if (c.size() <3 || c.size() > 4) return _false_return();
		if (c.size() == 4) continue;
		if (f1<0) f1 = i;
		else if (f2<0) f2 = i;
		else return _false_return();
	}
	if (f2<0) return _false_return();
	auto oi = is_opposite(data, f1, f2);
	if (oi < 0) return _false_return();
	auto dlower = data[f1], dupper = data[f2];
	std::rotate(dupper.begin(), dupper.begin() + oi, dupper.end());
	pts = {dlower[0], dlower[1], dlower[2], dupper[0], dupper[2], dupper[1]};
	return true;
}
bool hme::vtkcell_expression::try_polygon(std::vector<std::vector<int>>& data){
	celltype=7;
	if (data.size() != 1) return _false_return();
	pts = data[0];
	return true;
}
bool hme::vtkcell_expression::try_polyhedron(std::vector<std::vector<int>>& data){
	celltype=42;
	if (data.size() < 4) return _false_return();
	pts.clear();
	pts.push_back(data.size());
	for (auto& d: data){
		pts.push_back(d.size());
		for (auto& p: d) pts.push_back(p);
	}
	return true;
}

hme::vtkcell_expression hme::vtkcell_expression::build(std::vector<std::vector<int>>& cint){
	vtkcell_expression ret;
	if (ret.try_tetrahedron(cint)) return ret;
	if (ret.try_hexahedron(cint)) return ret;
	if (ret.try_wedge(cint)) return ret;
	if (ret.try_pyramid(cint)) return ret;
	if (ret.try_polygon(cint)) return ret;
	if (ret.try_polyhedron(cint)) return ret;
	std::string s("Can not treat 3D cell with ");
	s += std::to_string(cint.size());
	s += " faces as valid vtk format cell";
	throw std::runtime_error(s.c_str());
}
vector<hme::vtkcell_expression> hme::vtkcell_expression::cell_assembler(const Ser::Grid& ser,
		const vector<vector<int>>& aface, bool ignore_errors){
	const GridData& grid = ser.grid;
	vector<vtkcell_expression> ret; ret.reserve(ser.n_cells());
	grid.enumerate_all();

	for (int icell=0; icell<ser.n_cells(); ++icell){
		int len = grid.vcells[icell]->faces.size();
		//assemble cell->points
		vector<vector<int>> cell_points; cell_points.reserve(len);
		for (int j=0; j<len; ++j){
			//insert face data
			int iface = grid.vcells[icell]->faces[j]->id;
			cell_points.push_back(aface[iface]);
			//reverse to guarantee left cell
			int leftcell = ser.face_cell()[2*iface];
			if (leftcell != icell) {
				auto& vertv = cell_points.back();
				std::reverse(vertv.begin(), vertv.end());
			}
		}
		//match vtk data format. throws if impossible
		try{
			ret.push_back(vtkcell_expression::build(cell_points));
		} catch (std::runtime_error& e){
			if (!ignore_errors) throw;
			else ret.push_back(vtkcell_expression());
		}
	}
	return ret;
}


void hme::TGridVTK::_run(const Ser::Grid& ser, std::string fn){
	callback->step_after(20, "Assembling faces");
	vector<vector<int>> aface = ser.face_vertex();

	callback->step_after(20, "Assembling cells");
	vector< vtkcell_expression > vtkcell = hme::vtkcell_expression::cell_assembler(ser, aface);
	int nffull = std::accumulate(vtkcell.begin(), vtkcell.end(), 0, 
			[](int s, const vtkcell_expression& v){ return s + v.wsize(); });

	callback->silent_step_after(40, "Writing to file", 2, 0);
	//write to file:
	//header
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"3D Grid"<<std::endl;
	fs<<"ASCII"<<std::endl;

	//Points
	callback->subprocess_step_after(1);
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<ser.n_vert()<< " float"<<std::endl;
	auto& vert = ser.vert();
	for (int i=0; i<3*ser.n_vert(); i+=3)
		fs<<vert[i]<<" "<<vert[i+1]<<" "<<vert[i+2]<<std::endl;

	//Cells
	callback->subprocess_step_after(1);
	fs<<"CELLS  "<<vtkcell.size()<<"   "<<nffull<<std::endl;
	for (auto& f: vtkcell) fs<<f.to_string()<<std::endl;
	fs<<"CELL_TYPES  "<<vtkcell.size()<<std::endl;
	for (auto& f: vtkcell) fs<<f.stype()<<std::endl;

	fs.close();
}

void hme::TGridVTK::_run(const GridData& ser, std::string fn){
	return _run(Ser::Grid(ser), fn);
}

namespace {
struct bnd_face_data{
	bnd_face_data(const Ser::Grid& _ser): ser(&_ser){}

	//--- assembling steps
	void n1_extract_bfaces(){     //fills findices
		for (int i=0; i<ser->n_faces(); ++i){
			if (ser->grid.vfaces[i]->is_boundary()){
				findices.push_back(i);
			}
		}
	} 
	void n11_extract_boundaries(){ //fills fbtypes
		//fill non zero types
		const std::vector<int>& btvec = ser->btypes();
		
		//fill fbtypes
		fbtypes.reserve(n_faces());
		for (int i=0; i<n_faces(); ++i) fbtypes.push_back(btvec[findices[i]]);
	}
	void n12_assemble_gfv(){   //fill global_face_vertices
		global_face_vertices.reserve(n_faces());
		for (int i=0; i<n_faces(); ++i){
			int find = findices[i];
			global_face_vertices.push_back(ser->face_vertex(find));
		}
	}
	void n2_extract_bvert(){      //fills vindices
		std::vector<bool> used_v(ser->n_vert(), false);
		for (int i=0; i<n_faces(); ++i){
			vector<int>& fver = global_face_vertices[i];
			for (int j=0; j<fver.size(); ++j){
				int ind = fver[j];
				if (!used_v[ind]){
					used_v[ind] = true;
					vindices.push_back(ind);
				}
			}
		}
	}
	void n3_extract_bcells(){     //fills cindices
		cindices.reserve(n_faces());
		auto& fc = ser->face_cell();
		for (int i=0; i<n_faces(); ++i){
			int kf = findices[i];
			int c1 = fc[2*kf];
			if (c1 < 0) c1 = fc[2*kf+1];
			cindices.push_back(c1);
		}
	}
	void n4_vertices_raw(){     //fills vertices_raw
		vertices_raw.reserve(3*n_vert());
		for (int i=0; i<n_vert(); ++i){
			auto vstart = ser->vert().begin() + 3*vindices[i];
			vertices_raw.insert(vertices_raw.end(), vstart, vstart+3);
		}
	}
	void n5_faces_raw(){       //fills faces_raw
		//vertex addressing
		vector<int> vert_global_to_local(ser->n_vert(), -1);
		for (int i=0; i<n_vert(); ++i){
			vert_global_to_local[vindices[i]] = i;
		}
		
		//size of raw output
		int sz = n_faces();
		auto& fe = ser->face_edge();
		for (int i=0; i<n_faces(); ++i){
			int gi = findices[i];
			sz += (fe[gi].size());
		}

		//raw outpout
		faces_raw.reserve(sz);
		for (int i=0; i<n_faces(); ++i){
			int gi = findices[i];
			int len = fe[gi].size();
			faces_raw.push_back(len);
			auto start = global_face_vertices[i].begin();
			for (int j=0; j<len; ++j) {
				int vindex = vert_global_to_local[*start++];
				faces_raw.push_back(vindex);
			}
		}

	}

	//--- write data
	void write_points(std::ostream& fs){
		fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
		fs<<"POINTS "<<n_vert()<< " float"<<std::endl;
		for (int i=0; i<3*n_vert(); i+=3){
			fs<<(float)vertices_raw[i]<<" ";
			fs<<(float)vertices_raw[i+1]<<" ";
			fs<<(float)vertices_raw[i+2]<<"\n";
		}
	}
	void write_faces(std::ostream& fs){
		fs<<"CELLS  "<<n_faces()<<"   "<<faces_raw.size()<<std::endl;
		auto it = faces_raw.begin();
		for (int i=0; i<n_faces(); ++i){
			int n = *it;
			for (int j=0; j<n; ++j) fs<<*it++<<" "; fs<<*it++<<"\n";
		}
		fs<<"CELL_TYPES  "<<n_faces()<<std::endl;
		for (int i=0; i<n_faces();++i) fs<<7<<" "; fs<<std::endl;
	}
	void write_array(std::ostream& fs, const vector<int>& f, const char* name){
		fs<<"SCALARS "<<name<<" int 1"<<std::endl;
		fs<<"LOOKUP_TABLE default"<<std::endl;
		for (auto d: f) fs<<d<<" "; fs<<std::endl;
	}

	//--- aux data
	const Ser::Grid* ser;
	vector<vector<int>> global_face_vertices;
	
	//--- main data for output
	vector<double> vertices_raw;
	vector<int> faces_raw;
	vector<int> fbtypes;
	vector<int> cindices;
	vector<int> findices;
	vector<int> vindices;

	int n_vert() { return vindices.size(); }
	int n_faces() { return findices.size(); }
};
}

void hme::TBoundaryVTK::_run(const Ser::Grid& ser, std::string fn){
	bnd_face_data fdata(ser);
	callback->step_after(20, "Extract boundary", 4, 1);
	fdata.n1_extract_bfaces();
	fdata.n11_extract_boundaries();

	callback->subprocess_step_after(2);
	fdata.n12_assemble_gfv();
	fdata.n2_extract_bvert();

	callback->subprocess_step_after(1);
	fdata.n3_extract_bcells();
	callback->subprocess_fin();

	callback->step_after(10, "Renumber vertices");
	fdata.n4_vertices_raw();

	callback->step_after(10, "Assemble faces");
	fdata.n5_faces_raw();

	callback->silent_step_after(10, "Write to file", 3);
	//write to file:
	//header
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"Boundary for 3D Grid"<<std::endl;
	fs<<"ASCII"<<std::endl;

	//points
	callback->subprocess_step_after(1);
	fdata.write_points(fs);

	//faces
	callback->subprocess_step_after(1);
	fdata.write_faces(fs);

	//additional info
	callback->subprocess_step_after(1);
	fs<<"POINT_DATA"<<" "<<fdata.n_vert()<<std::endl;
	fdata.write_array(fs, fdata.vindices, "vertex_global_indices");
	fs<<"CELL_DATA"<<" "<<fdata.n_faces()<<std::endl;
	fdata.write_array(fs, fdata.findices, "face_global_indices");
	fdata.write_array(fs, fdata.cindices, "adjacent_cell_indices");
	fdata.write_array(fs, fdata.fbtypes, "boundary_type");

	fs.close();
}
void hme::TBoundaryVTK::_run(const GridData& ser, std::string fn){
	return _run(Ser::Grid(ser), fn);
}
void hme::TAllVTK::_run(const Ser::Grid& g, std::string fngrid, std::string fnbnd){
	GridVTK.MoveCallback(*callback, g, fngrid);
	BoundaryVTK.MoveCallback(*callback, g, fnbnd);
}
void hme::TAllVTK::_run(const GridData& g, std::string fngrid, std::string fnbnd){
	return _run(Ser::Grid(g), fngrid, fnbnd);
}
void hme::TSurfaceVTK::_run(const Ser::Surface& s, std::string fn){
	//header
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"3D Surface"<<std::endl;
	fs<<"ASCII"<<std::endl;

	callback->step_after(40, "Writing vertices");
	//points
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<s.n_vert()<< " float"<<std::endl;
	auto& vert = s.vert();
	for (int i=0; i<3*s.n_vert(); i+=3){
		fs<<(float)vert[i+0]<<" ";
		fs<<(float)vert[i+1]<<" ";
		fs<<(float)vert[i+2]<<"\n";
	}

	//faces
	callback->step_after(30, "Writing faces");
	int raw_dim = s.n_faces();
	auto& fe = s.face_edge();
	for (int i=0; i<s.n_faces(); ++i) raw_dim += fe[i].size();
	fs<<"CELLS  "<<s.n_faces()<<"   "<<raw_dim<<std::endl;
	for (int i=0; i<s.n_faces(); ++i){
		auto& fv = s.face_vertex(i);
		fs<<fv.size();
		for (int j=0; j<fv.size(); ++j) fs<<" "<<fv[j]; fs<<"\n";
	}
	fs<<"CELL_TYPES  "<<s.n_faces()<<std::endl;
	for (int i=0; i<s.n_faces();++i) fs<<7<<" "; fs<<std::endl;

	//additional info
	callback->step_after(10, "Boundary types");
	fs<<"CELL_DATA"<<" "<<s.n_faces()<<std::endl;
	fs<<"SCALARS boundary_type int 1"<<std::endl;
	fs<<"LOOKUP_TABLE default"<<std::endl;
	auto& bt = s.btypes();
	for (int i=0; i<s.n_faces(); ++i){
		fs<<bt[i]<<std::endl;
	}

	fs.close();
}
void hme::TSurfaceVTK::_run(const FaceData& s, std::string fn){
	callback->step_after(20, "Serializing data");
	SurfaceVTK.MoveCallback(*callback, Ser::Surface(s), fn);
}
