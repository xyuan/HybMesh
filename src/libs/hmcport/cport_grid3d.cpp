#include "hmcport.h"
#include "cport_grid3d.h"
#include "c2cpp_helper.hpp"
#include "cport_surface3d.h"
#include "tscaler.hpp"
#include "primitives3d.hpp"
#include "surface.hpp"
#include "treverter3d.hpp"
#include "merge3d.hpp"
#include "construct_grid3d.hpp"
#include "revolve_grid3d.hpp"
#include "tetrahedral.hpp"
#include "export3d_vtk.hpp"
#include "export3d_fluent.hpp"
#include "export3d_gmsh.hpp"
#include "export3d_tecplot.hpp"
#include "export3d_hm.hpp"


int g3_move(void* obj, double* dx){
	try{
		for (auto& v: static_cast<HM3D::GridData*>(obj)->vvert){
			v->x += dx[0];
			v->y += dx[1];
			v->z += dx[2];
		}
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_scale(void* obj, double* pc, double* p0){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		double xc = pc[0] / 100., yc = pc[1] / 100., zc = pc[2] / 100.;
		for (auto& p: g->vvert){
			p->x -= p0[0]; p->x *= xc;
			p->y -= p0[1]; p->y *= yc;
			p->z -= p0[2]; p->z *= zc;
		}
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_tab_btypes(void* obj, int* ret){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		return s3_tab_btypes(&g->vfaces, ret);
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_point_at(void* obj, int index, double* ret){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		if (g->vvert.size()>=index) throw std::runtime_error("index is out of range");
		ret[0] = g->vvert[index]->x;
		ret[1] = g->vvert[index]->y;
		ret[2] = g->vvert[index]->z;
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

int g3_deepcopy(void* obj, void** ret){
	try{
		HM3D::GridData ret_;
		HM3D::DeepCopy(*static_cast<HM3D::GridData*>(obj), ret_);
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_concatenate(int nobjs, void** objs, void** ret){
	try{
		auto gg = c2cpp::to_pvec<HM3D::GridData>(nobjs, objs);
		HM3D::GridData ret_;
		for (auto g: gg){
			ret_.vvert.insert(ret_.vvert.end(), g->vvert.begin(), g->vvert.end());
			ret_.vedges.insert(ret_.vedges.end(), g->vedges.begin(), g->vedges.end());
			ret_.vfaces.insert(ret_.vfaces.end(), g->vfaces.begin(), g->vfaces.end());
			ret_.vcells.insert(ret_.vcells.end(), g->vcells.begin(), g->vcells.end());
		}
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//====== destructor
int g3_free(void* obj){
	try{
		delete static_cast<HM3D::GridData*>(obj);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//====== information
//n_vert, n_edges, n_faces, n_cells
int g3_dims(void* obj, int* ret){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		ret[0] = g->vvert.size();
		ret[1] = g->vedges.size();
		ret[2] = g->vfaces.size();
		ret[3] = g->vcells.size();
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//n_vert, n_edges, n_faces
int g3_bnd_dims(void* obj, int* dims){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		auto s = HM3D::Surface::GridSurface(*g);
		return s3_dims(&s, dims);
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//boundary area
int g3_bnd_area(void* obj, double* ret){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		auto c = HM3D::Surface::GridSurface(*g);
		return s3_area(&c, ret);
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//creates surface out of grid boundary
int g3_extract_surface(void* obj, void** ret){
	try{
		auto surf = HM3D::Surface::GridSurface(
			*static_cast<HM3D::GridData*>(obj));
		return s3_deepcopy(&surf, ret);
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//volume
int g3_volume(void* obj, double* ret){
	try{
		auto g = static_cast<HM3D::GridData*>(obj);
		auto srf = HM3D::Surface::GridSurface(*g);
		HM3D::Surface::R::RevertGridSurface rv(srf, true);
		*ret = HM3D::Surface::Volume(srf);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//merge coincident primitives
int g3_merge(void* obj1, void* obj2, void** ret, hmcport_callback cb){
	try{
		auto g1 = static_cast<HM3D::GridData*>(obj1);
		auto g2 = static_cast<HM3D::GridData*>(obj2);
		Autoscale::D3 sc(vector<HM3D::GridData*>{g1, g2});
		HM3D::GridData ret_ = HM3D::MergeGrids(*g1, *g2);
		sc.unscale(&ret_);
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

int g3_extrude(void* obj, int nz, double* zvals,
		int*  btop, int* bbot,
		int bside, void** ret){
	try{
		auto g2 = static_cast<HM2D::GridData*>(obj);
		vector<double> z(zvals, zvals+nz);
		auto topfun = [btop](int i){ return btop[i]; };
		auto botfun = [bbot](int i){ return bbot[i]; };
		HM3D::GridData ret_;
		if (bside >= 0){
			ret_ = HM3D::Constructor::SweepGrid2D(
					*g2, z, botfun, topfun, bside);
		} else {
			ret_ = HM3D::Constructor::SweepGrid2D(
					*g2, z, botfun, topfun);
		}
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

//vec - [x0, y0, x1, y1] array defining vector of rotation
//phi[n_phi] - increasing vector of angular partition (degree)
//b1, b2 - boundary types for surfaces at minimum and maximum phi's for
//         incoplete rotation grids
//is_trian (bool) - whether to triangulate center cell
//return NULL if failed
int g3_revolve(void* obj, double* vec, int n_phi, double* phi,
		int is_trian, int b1, int b2, void** ret){
	try{
		HM2D::GridData* g2 = static_cast<HM2D::GridData*>(obj);
		Point pstart(vec[0], vec[1]), pend(vec[2], vec[3]);
		Autoscale::D2 sc(g2);
		sc.scale(pstart);
		sc.scale(pend);
		std::vector<double> vphi(phi, phi + n_phi);

		HM3D::GridData ret_ = HM3D::Constructor::RevolveGrid2D(
			*g2, vphi, pstart, pend, is_trian, b1, b2);
		sc.unscale(&ret_);
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}


//======= unstructured fill
//0 on fail
int g3_tetrahedral_fill(int nsurf, void** surf,
		int nconstr, void** constr,
		int npts, double* pcoords, double* psizes,
		void** ret, hmcport_callback cb){
	try{
		//TODO constraints are not supported yet
		auto surf_ = c2cpp::to_pvec<HM3D::FaceData>(nsurf, surf);
		//scaling
		Autoscale::D3 sc(surf_);
		//collect all input source surfaces
		HM3D::FaceData source;
		for (auto& s: surf_) source.insert(source.end(), s->begin(), s->end());
		//procedure
		HM3D::GridData ret_ = HM3D::Mesher::UnstructuredTetrahedral.WithCallback(
				cb, source);
		sc.unscale(&ret_);
		c2cpp::to_pp(ret_, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}


//====== exporters
int g3_to_vtk(void* obj, const char* fname, hmcport_callback f2){
	try{
		HM3D::Export::GridVTK.WithCallback(f2,
			*static_cast<HM3D::GridData*>(obj),
			fname);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_surface_to_vtk(void* obj, const char* fname, hmcport_callback f2){
	try{
		HM3D::Export::BoundaryVTK.WithCallback(f2,
			*static_cast<HM3D::GridData*>(obj),
			fname);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}

namespace{
HM3D::Export::BFun construct_bnames(const BoundaryNamesStruct& bnames){
	std::map<int, std::string> bnames_map;
	for (int i=0; i<bnames.n; ++i){
		bnames_map[bnames.index[i]] = std::string(bnames.name[i]);
	}
	auto fnames = [bnames_map](int i)->std::string{
			auto fnd = bnames_map.find(i);
			if (fnd == bnames_map.end()) return "boundary" + std::to_string(i);
			else return fnd->second;
		};
	return fnames;
}
}

int g3_to_msh(void* obj, const char* fname, BoundaryNamesStruct bnames,
		int n_periodic, double* data_periodic, hmcport_callback f2){
	try{
		// name function
		auto nmfunc = construct_bnames(bnames);
		// building periodic
		HM3D::Export::PeriodicData pd;
		for (int i=0; i<n_periodic; ++i){
			int b1 = (int)(*data_periodic++);
			int b2 = (int)(*data_periodic++);
			double x1 = *data_periodic++;
			double y1 = *data_periodic++;
			double z1 = *data_periodic++;
			double x2 = *data_periodic++;
			double y2 = *data_periodic++;
			double z2 = *data_periodic++;
			pd.add_condition(b1, b2, HM3D::Vertex(x1, y1, z1), HM3D::Vertex(x2, y2, z2), true);
		}
		//call function
		auto g = static_cast<HM3D::GridData*>(obj);
		if (pd.size() == 0){
			HM3D::Export::GridMSH.WithCallback(f2, *g, fname, nmfunc);
		} else {
			HM3D::Export::GridMSH.WithCallback(f2, *g, fname, nmfunc, pd);
		}
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_to_gmsh(void* obj, const char* fname, BoundaryNamesStruct bnames,
		hmcport_callback f2){
	try{
		// name function
		auto nmfunc = construct_bnames(bnames);
		//call function
		auto g = static_cast<HM3D::GridData*>(obj);
		HM3D::Export::GridGMSH.WithCallback(f2, *g, fname, nmfunc);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_to_tecplot(void* obj, const char* fname, BoundaryNamesStruct bnames,
		hmcport_callback f2){
	try{
		// name function
		auto nmfunc = construct_bnames(bnames);
		//call function
		auto g = static_cast<HM3D::GridData*>(obj);
		HM3D::Export::GridTecplot.WithCallback(f2, *g, fname, nmfunc);
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
int g3_to_hm(void* doc, void* node, void* obj, const char* name, const char* fmt,
		int naf, const char** af,
		hmcport_callback f2){
	try{
		HMXML::ReaderA* wr = static_cast<HMXML::ReaderA*>(doc);
		HMXML::Reader* sn = static_cast<HMXML::Reader*>(node);
		HM3D::GridData* g = static_cast<HM3D::GridData*>(obj);
		HM3D::Export::GridWriter gw(*g, wr, sn, name, fmt);

		//additional fields
		for (int i=0; i<naf; ++i){
			switch (std::map<const char*, int>{
					{"face_vertices", 1},
					{"face-vertices", 2},
					{"cell_faces", 3},
					{"cell-faces", 4},
					{"cell_vertices", 5},
					{"cell-vertices", 6},
					{"linfem", 7} }[af[i]]){
			case 1: case 2:
				gw.AddFaceVertexConnectivity();
				break;
			case 3: case 4:
				gw.AddCellFaceConnectivity();
				break;
			case 5: case 6:
				gw.AddCellVertexConnectivity();
				break;
			case 7:
				gw.AddLinFemConnectivity();
				break;
			default:
				throw std::runtime_error("unknown field "+std::string(af[i]));
			}
		}
		return HMSUCCESS;
	} catch (std::exception& e){
		std::cout<<e.what()<<std::endl;
		return HMERROR;
	}
}
