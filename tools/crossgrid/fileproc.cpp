#include "fileproc.h"
#include <fstream>

void save_vtk(const GridGeom* g, const char* fn){
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"Grid 2D"<<std::endl;
	fs<<"ASCII"<<std::endl;
	//Points
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<g->n_points()<< " float"<<std::endl;
	for (int i=0;i<g->n_points();++i){
		auto p = g->get_point(i);
		fs<<p->x<<" "<<p->y<<" 0"<<std::endl;
	}
	//Cells
	fs<<"CELLS  "<<g->n_cells()<<"   "<<g->n_cells()+g->n_cellsdim()<<std::endl;
	for (int i=0;i<g->n_cells();++i){
		auto c = g->get_cell(i);
		fs<<c->dim()<<"  ";
		for (int j=0;j<c->dim();++j)
			fs<<c->get_point(j)->get_ind()<<" ";
		fs<<std::endl;
	}
	fs<<"CELL_TYPES  "<<g->n_cells()<<std::endl;
	for (int i=0;i<g->n_cells();++i) fs<<7<<std::endl;
	fs.close();
}

void save_vtk(const PContour* c, const char* fn){
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"Contour 2D"<<std::endl;
	fs<<"ASCII"<<std::endl;
	//Points
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<c->n_points()<< " float"<<std::endl;
	for (int i=0;i<c->n_points();++i){
		auto p = c->get_point(i);
		fs<<p->x<<" "<<p->y<<" 0"<<std::endl;
	}
	//Cells
	fs<<"CELLS  "<<c->n_points()<<"   "<<3*c->n_points()<<std::endl;
	for (int i=0;i<c->n_points();++i){
		int inext = (i+1) % c->n_points();
		fs<<2<<" "<<i<<" "<<inext<<std::endl;
	}
	fs<<"CELL_TYPES  "<<c->n_points()<<std::endl;
	for (int i=0;i<c->n_points();++i) fs<<3<<std::endl;
	fs.close();
}

void save_vtk(const vector<PContour>& c, const char* fn){
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"Contour 2D"<<std::endl;
	fs<<"ASCII"<<std::endl;
	//Points
	int numpts = 0;
	for (auto cont: c) numpts+=cont.n_points();
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<numpts<< " float"<<std::endl;
	for (auto cont: c){
		for (int i=0;i<cont.n_points();++i){
			auto p = cont.get_point(i);
			fs<<p->x<<" "<<p->y<<" 0"<<std::endl;
		}
	}
	//Cells
	fs<<"CELLS  "<<numpts<<"   "<<3*numpts<<std::endl;
	int globN = 0;
	for (auto cont: c){
		for (int i=0;i<cont.n_points();++i){
			int inext = (i+1) % cont.n_points();
			fs<<2<<" "<<i+globN<<" "<<inext+globN<<std::endl;
		}
		globN+=cont.n_points();
	}
	fs<<"CELL_TYPES  "<<numpts<<std::endl;
	for (int i=0;i<numpts;++i) fs<<3<<std::endl;
	fs.close();
}

void save_vtk(const vector<PContour>& c, const vector<double>& data, const char* fn){
	save_vtk(c, fn);
	std::ofstream f(fn, std::ios_base::app);
	f<<"POINT_DATA "<<data.size()<<std::endl;
	f<<"SCALARS data float 1"<<std::endl;
	f<<"LOOKUP_TABLE default"<<std::endl;
	for (auto v: data) f<<v<<std::endl;
	f.close();
}

void save_vtk(const PtsGraph* g, const char* fn){
	std::ofstream fs(fn);
	fs<<"# vtk DataFile Version 3.0"<<std::endl;
	fs<<"PtsGraph"<<std::endl;
	fs<<"ASCII"<<std::endl;
	//Points
	fs<<"DATASET UNSTRUCTURED_GRID"<<std::endl;
	fs<<"POINTS "<<g->Nnodes()<< " float"<<std::endl;
	for (int i=0;i<g->Nnodes();++i){
		auto p = g->get_point(i);
		fs<<p->x<<" "<<p->y<<" 0"<<std::endl;
	}
	//Cells
	fs<<"CELLS  "<<g->Nlines()<<"   "<<3*g->Nlines()<<std::endl;
	for (int i=0;i<g->Nlines();++i){
		auto ln = g->get_line(i);
		fs<<2<<" "<<ln.first<<" "<<ln.second<<std::endl;
	}
	fs<<"CELL_TYPES  "<<g->Nlines()<<std::endl;
	for (int i=0;i<g->Nlines();++i) fs<<3<<std::endl;
	fs.close();
}
