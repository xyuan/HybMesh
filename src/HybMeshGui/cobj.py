' procedures for communication with c libraries'
import ctypes as ct
import globvars
import grid2
import bgeom


def crossgrid_lib():
    libfn = globvars.prog_options.lib_crossgrid
    return ct.cdll.LoadLibrary(libfn)


def cont2_lib():
    libfn = globvars.prog_options.lib_cont2
    return ct.cdll.LoadLibrary(libfn)


def grid_to_c(g):
    " return c pointer to a python grid"
    lib_fa = crossgrid_lib()
    npt = g.n_points()
    ncls = g.n_cells()
    #fill points
    c_pnt = (ct.c_double * (2 * npt))()
    for i, p in enumerate(g.points):
        c_pnt[2 * i] = p.x
        c_pnt[2 * i + 1] = p.y
    #fill cells
    cls_a = g.cells_nodes_connect()
    cls_a_num = len(cls_a)
    for ce in cls_a:
        cls_a_num += len(ce)
    c_cls = (ct.c_int * cls_a_num)()
    ind = 0
    for ce in cls_a:
        c_cls[ind] = len(ce)
        for i in range(1, len(ce) + 1):
            c_cls[ind + i] = ce[i - 1]
        ind += (len(ce) + 1)
    return lib_fa.grid_construct(ct.c_int(npt), ct.c_int(ncls), c_pnt, c_cls)


def cont_to_c(c):
    """ return c pointer to a contours2.Contour2 object
        works for contours of crossgrid library.
        Will be removed
    """
    lib_fa = crossgrid_lib()
    #fill points
    c_pnt = (ct.c_double * (2 * c.n_points()))()
    for i, p in enumerate(c.points):
        c_pnt[2 * i] = p.x
        c_pnt[2 * i + 1] = p.y
    #fill edges
    edg = c.edges_points()
    c_edg = (ct.c_int * (2 * c.n_edges()))()
    for i, ce in enumerate(edg):
        c_edg[2 * i] = ce[0]
        c_edg[2 * i + 1] = ce[1]

    return lib_fa.contour_construct(ct.c_int(c.n_points()),
            ct.c_int(c.n_edges()), c_pnt, c_edg)


def grid_from_c(c_gr):
    "builds a grid from a c object"
    lib_fa = crossgrid_lib()
    #c types
    ct_pint = ct.POINTER(ct.c_int)
    ct_ppint = ct.POINTER(ct.POINTER(ct.c_int))
    ct_pd = ct.POINTER(ct.c_double)
    ct_ppd = ct.POINTER(ct.POINTER(ct.c_double))
    #c data allocation
    npt, ned, ncl = ct.c_int(), ct.c_int(), ct.c_int()
    pt, ed, cdims, ced = ct_pd(), ct_pint(), ct_pint(), ct_pint()
    #call c function
    lib_fa.grid_get_edges_info.argtypes = [ct.c_void_p, ct_pint, ct_pint,
            ct_pint, ct_ppd, ct_ppint, ct_ppint, ct_ppint]
    lib_fa.grid_get_edges_info(c_gr, ct.byref(npt), ct.byref(ned),
            ct.byref(ncl), ct.byref(pt), ct.byref(ed),
            ct.byref(cdims), ct.byref(ced))
    # ---- construct grid
    ret = grid2.Grid2()
    #points
    it = iter(pt)
    for i in range(npt.value):
        ret.points.append(bgeom.Point2(next(it), next(it)))
    #edges
    it = iter(ed)
    for i in range(ned.value):
        ret.edges.append([next(it), next(it)])
    #cells
    it1, it2 = iter(cdims), iter(ced)
    for i in range(ncl.value):
        ied = [next(it2) for j in range(next(it1))]
        ret.cells.append(ied)
    #free c memory
    lib_fa.grid_free_edges_info.argtypes = [ct_ppd, ct_ppint, ct_ppint,
            ct_ppint]
    lib_fa.grid_free_edges_info(ct.byref(pt), ct.byref(ed), ct.byref(cdims),
            ct.byref(ced))

    return ret


def cont2_to_c(cont):
    """ return c pointer to a contours2.AbstractContour2 objects
        works for contours of hybmesh_contours2 library.
    """
    lib_c2 = cont2_lib()
    # arguments
    c_npnt = ct.c_int(cont.n_points())
    c_pnts = (ct.c_double * (2 * c_npnt.value))()
    c_nedges = ct.c_int(cont.n_edges())
    for i, p in enumerate(cont.points):
        c_pnts[2 * i] = p.x
        c_pnts[2 * i + 1] = p.y

    c_edges = (ct.c_int * (3 * c_nedges.value))()
    for i, [i0, i1, b] in enumerate(cont.edges_points()):
        c_edges[3 * i] = i0
        c_edges[3 * i + 1] = i1
        c_edges[3 * i + 2] = b

    return lib_c2.create_contour_tree(c_npnt, c_pnts,
        c_nedges, c_edges)


