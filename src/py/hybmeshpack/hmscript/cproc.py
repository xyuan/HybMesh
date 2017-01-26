import numbers
import copy
from hybmeshpack import com
from hybmeshpack.basic.geom import Point2
from hybmeshpack.hmscript import flow
from hybmeshpack.hmscript import ExecError
from hybmeshpack.hmcore import c2 as c2core
from hybmeshpack.hmscript import datachecks as dch


def grid_bnd_to_contour(g1, simplify=True):
    """ Extracts grid boundary to user contour

    :param g1: grid identifier

    :param bool simplify: if true deletes all non-significant points. Otherwise
       resulting contour will contain all boundary grid edges.

    :returns: contour identifier
    """
    c = com.contcom.GridBndToContour({"grid_name": g1,
                                      "simplify": simplify})
    flow.exec_command(c)
    return c._get_added_names()[1][0]


def grid3_bnd_to_surface(g1, separate=False):
    """ Returns surface object built out of grid boundary

    :param g1: grid identifier

    :param bool separate: whether grid surface should be separated
       into singly connected set of surfaces

    :returns: grid identifier if **separate** is False or
       list of grid identifiers otherwise

    :raises: ValueError, ExecError

    """
    c = com.surfcom.Grid3BndToSurface({"grid_name": g1,
                                      "separate": separate})
    try:
        flow.exec_command(c)
        if separate:
            return c._get_added_names()[3]
        else:
            return c._get_added_names()[3][0]
    except Exception:
        raise ExecError("grid3_bnd_to_surface")


def simplify_contour(cont, simplify=True, angle=0, separate=False):
    """ Separates and simplify user contour

    :param cont: source contour or grid identifier

    :param bool simplify: do simplification, i.e. make all
       segments non-collinear
       Collinear segments will not be splitted if they have different
       boundary types.

    :param degree angle: minimum allowed angle between segments
       after simplification (deg, >=0)

    :param bool separate: assemble list of singly connected contours from
       multiply connected source contour

    :returns: list of created contours ids

    """
    c = com.contcom.SimplifyContours({"names": [cont],
                                      "simplify": simplify,
                                      "angle": angle,
                                      "separate": separate})
    flow.exec_command(c)
    return c._get_added_names()[1]


def decompose_contour(cont):
    """ Returns set of simple singly connected contours built from
    input contour

    :param cont: contour identifier

    :returns: list of new contour identifiers

    :raises: ValueError, ExecError

    All resulting contour vertices will have no more than two adjacent edges.
    If input contour has vertices with more than two connections
    (or self intersections) it will be splitted at these points.

    Tries to assemble as many closed contours as possible.
    """
    c = com.contcom.DecomposeContour({"source": cont})
    try:
        flow.exec_command(c)
        return c._get_added_names()[1]
    except:
        raise ExecError("decompose_contour")


def unite_contours(conts):
    """ Unites contours to single multiply connected contour

    :param conts: list of contours identifiers to unite

    :return: contour identifier

    Equal nodes of input contours will be merged.
    """
    c = com.contcom.UniteContours({"sources": conts})
    flow.exec_command(c)
    return c._get_added_names()[1][0]


def add_boundary_type(index, name="boundary1"):
    """ Creates boundary type

    :param int index: index of boundary (>0)

    :param str name: user defined name of the boundary

    :returns: integer boundary identifier

    If boundary with ``index`` already exists it will be overwritten.
    Name of the boundary should be unique, if it already exists it will
    be changed automatically.

    """
    c = com.contcom.EditBoundaryType({"index": index, "name": name})
    flow.exec_command(c)
    return index


def set_boundary_type(cont, btps=None, bfun=None):
    """ Mark user or grid contour segments with boundary types.

    :param cont: contour or grid identifier

    :param btps: list of boundary types identifiers for each segment
       or single identifier for the whole contour

    :param  bfun: function ``(x0, y0, x1, y1, bt) -> btype``
       which returns boundary type taking segment endpoints
       and old boundary type as arguments.

    Example:

      .. literalinclude:: ../../testing/py/fromdoc/ex_setbtype.py
          :start-after: START OF EXAMPLE
          :end-before: END OF EXAMPLE

    Only one of **btps**, **bfun** arguments should be defined.
    """
    cdata = flow.get_receiver().get_any_contour(cont)
    args = {"name": cont}
    if btps is not None:
        if not isinstance(btps, list):
            args[btps] = range(cdata.n_edges())
        else:
            for i, b in enumerate(btps):
                if b not in args:
                    args[b] = []
                args[b].append(i)
    elif bfun is not None:
        ep = cdata.edges_points()
        for i, iep in enumerate(ep):
            p0 = cdata.points[iep[0]]
            p1 = cdata.points[iep[1]]
            r = bfun(p0.x, p0.y, p1.x, p1.y, cdata.edge_bnd(i))
            if r is None:
                continue
            if r not in args:
                args[r] = []
            args[r].append(i)

    c = com.contcom.SetBTypeToContour({"btypes": [args]})
    flow.exec_command(c)


def create_contour(pnts, bnds=0):
    """ Create singly connected contour from sequence of points

    :param list-of-list-of-floats pnts: sequence of points.
       If coordinates of first and last points are equal
       then contour is considered closed.

    :param single-or-list-of-boundary-identifiers bnds: boundary type for
       each contour segment or single identifier for the whole contour.

    :returns: contour identifier

    Example:
       >>> hmscript.create_contour([[0, 0], [1, 0], [1, 1], [0, 0]],
                                  [b1, b2, b3])
    """
    pt = []
    for p in pnts:
        pt.append(Point2(*p))
    c = com.contcom.CreateContour({"points": pt,
                                   "bnds": bnds})
    flow.exec_command(c)
    return c._get_added_names()[1][0]


def create_spline_contour(pnts, bnds=0, nedges=100):
    """ Creates singly connected contour as a parametric cubic spline.

    :param list-of-list-of-floats pnts: sequence of points.
         If coordinates of first and last points are equal
         then resulting contour will be closed.

    :param single-or-list-of-boundary-identifiers bnds: boundary type for
         each contour segment or single identifier for the whole contour.

    :param int nedges: number of line segments of resulting contour.
         Should be equal or greater than the number of sections defined by
         **pnts**.

    :returns: contour identifier

    :raises: hmscript.ExecError, ValueError
    """
    bad = False
    if not isinstance(pnts, list) or len(pnts) < 2:
        bad = True
    elif not all([isinstance(x, list) and len(x) == 2 for x in pnts]):
        bad = True
    if bad:
        raise ValueError("Invalid pnts data")
    if not isinstance(nedges, numbers.Integral) or nedges < len(pnts):
        raise ValueError("Invalid nedges")
    if not isinstance(bnds, numbers.Integral):
        if not isinstance(bnds, list):
            bad = True
        elif not all([isinstance(x, numbers.Integral) for x in bnds]):
            bad = True
    else:
        bnds = [bnds]
    if bad:
        raise ValueError("Invalid bnds")
    pt = []
    for p in pnts:
        pt.append(Point2(*p))
    c = com.contcom.CreateSpline({"points": pt,
                                  "bnds": bnds,
                                  "nedges": nedges})
    try:
        flow.exec_command(c)
        return c._get_added_names()[1][0]
    except:
        raise ExecError("create spline")


def clip_domain(dom1, dom2, operation, simplify=True):
    """ Executes domain clipping procedure

    :param dom1:

    :param dom2: contour identifiers

    :param  str operatrion: operation code

       * ``"union"``
       * ``"difference"``
       * ``"intersection"``
       * ``"xor"``

    :param bool simplify: whether to keep all source points (False) or
       return simplified contour

    :returns: created contour identifier or None if resulting domain is empty
    """
    if operation not in ['union', 'difference', 'intersection', 'xor']:
        raise ValueError("unknows operation: %s" % str(operation))
    c = com.contcom.ClipDomain({"c1": dom1, "c2": dom2, "oper": operation,
                                "simplify": simplify})
    flow.exec_command(c)
    if len(c._get_added_names()[1]) > 0:
        return c._get_added_names()[1][0]
    else:
        return None


def partition_contour(cont, algo, step=1, angle0=30, keep_bnd=False,
                      nedges=None, crosses=[], keep_pts=[],
                      start=None, end=None):
    """ Makes connected contour partition

    :param cont: Contour or grid identifier

    :param str algo: Partition algorithm:

       * ``'const'``: partition with defined constant step
       * ``'ref_points'``: partition with step function given by a
         set of values refered to basic points
       * ``'ref_weights'``: partition with step function given by a
         set of values refered to local contour [0, 1] coordinate
       * ``'ref_lengths'``: partition with step function given by a
         set of values refered to local contour length coordinate

    :param step: For ``algo='const'`` a float number defining
       partition step;

       For ``algo='ref_points'`` - list of step values and point coordinates
       given as
       ``[ step_0, [x_0, y_0], step_1, [x_1, y_1], ....]``.

       For ``algo='ref_weights'`` - list of step values and point normalized
       1d coordinates given as
       ``[ step_0, w_0, step_1, w_1, ....]``.

       For ``algo='ref_lengths'`` - list of step values and point 1d
       coordinates given as
       ``[ step_0, s_0, step_1, s_1, ....]``. Negative ``s_i`` shows
       length coordinate started from end point in reversed direction.


    :param float angle0: existing contour vertices which provide
       turns outside of ``[180 - angle0, 180 + angle0]`` degrees range
       will be preserved regardless of other options

    :param bool keep_bnd: if that is True than vertices which have different
       boundary features on their right and left sides will be preserved

    :param int nedges: if this parameter is not None then it provides
       exact number of edges in the resulting contour. To satisfy this
       condition **step** value will be multiplied by an appropriate factor.
       If it can not be satisfied (due to other restrictions)
       then an exception will be raised.

    :param crosses: represents set of contour which cross points with
       target contour will be present in resulting contour.

    :param keep_pts: list of points as ``[[x1, y1], [x2, y2], ...]`` list
       which should present in output partition.

    :param start:

    :param end: start and end points which define processing segment

    :returns: new contour identifier

    :raises: hmscript.ExecError, ValueError

    Points set defined by user for ``algo='ref_points'`` algorithm
    will not present in resulting contour (as well as points defined
    implicitly by other ``'ref_'`` algorithms). It just shows locations
    where step size of given length should be applied. If any point
    of this set is not located on the input contour then it will be
    projected to it.

    For constant stepping any contour including multiply connected ones
    could be passed. For ``ref_`` stepping only singly connected
    contours (open or closed) are allowed.

    If **start** and **end** points are defined then only segment between
    these points will be parted. Note that all closed contours are treated
    in counterclockwise direction. Given start/end points will be
    projected to closest contour vertex.

    ``ref_weights``, ``ref_lengths`` partition methods
    require definition of **start** point
    (To process whole contour ``end`` point could be omitted).

    Example:

      .. literalinclude:: ../../testing/py/fromdoc/ex_partcontour.py
          :start-after: vvvvvvvvvvvvvvvvvvvvvvvv
          :end-before: ^^^^^^^^^^^^^^^^^^^^^^^^

    See also: :ref:`simplecontmeshing`
    """
    # checks
    if algo == "const":
        if not isinstance(step, numbers.Real):
            raise ValueError("invalid step for const contour partition")
    elif algo == "ref_points":
        errs = "invalid step for ref_points partition"
        if not isinstance(step, list) or len(step) % 2 != 0:
            raise ValueError(errs)
        for i in range(len(step) / 2):
            if not isinstance(step[2 * i], numbers.Real):
                raise ValueError(errs)
            if not isinstance(step[2 * i + 1], list) or\
                    len(step[2 * i + 1]) != 2 or\
                    not isinstance(step[2 * i + 1][0], numbers.Real) or\
                    not isinstance(step[2 * i + 1][1], numbers.Real):
                raise ValueError(errs)
    elif algo in ["ref_weights", "ref_lengths"]:
        errs = "invalid step for ref_* partition"
        if not isinstance(step, list) or len(step) % 2 != 0:
            raise ValueError(errs)
        if start is None:
            raise ValueError("Define start point for ref_* partition")
        for i in range(len(step) / 2):
            if not isinstance(step[2 * i], numbers.Real):
                raise ValueError(errs)
            if not isinstance(step[2 * i + 1], numbers.Real):
                raise ValueError(errs)
        if algo == "ref_weights":
            for i in range(len(step) / 2):
                if 0 < step[2 * i + 1] > 1:
                    raise ValueError("Weight coordinate is out of [0, 1]")
        if algo == "ref_lengths":
            from hybmeshpack.hmscript.info import info_contour
            contlen = info_contour(cont)['length']
            for i in range(len(step) / 2):
                if 0 < abs(step[2 * i + 1]) > contlen:
                    raise ValueError("Length coordinate is out of [-L, L]")
    else:
        raise ValueError("unknown partition angorithm")
    if nedges is not None:
        if not isinstance(nedges, numbers.Integral):
            raise ValueError("invalid nedges value")
    if not isinstance(crosses, list):
        crosses = [crosses]
    for c in crosses:
        if not isinstance(c, str):
            raise ValueError("invalid crosses value")
        if c == cont:
            raise ValueError("Crossed contour equals target contour")
    # prepare arguments for command
    if algo == "const":
        plain_step = [step]
    elif algo == "ref_points":
        plain_step = []
        for i in range(len(step) / 2):
            plain_step.append(step[2 * i])
            plain_step.extend(step[2 * i + 1])
    elif algo in ["ref_weights", "ref_lengths"]:
        plain_step = copy.deepcopy(step)
    else:
        raise ValueError
    sp, ep = None, None
    if start is not None:
        sp = Point2(start[0], start[1])
    if end is not None:
        ep = Point2(end[0], end[1])
    kp = []
    if keep_pts is not None:
        for p in keep_pts:
            kp.append(Point2(p[0], p[1]))

    args = {"algo": algo,
            "step": plain_step,
            "angle0": angle0,
            "keepbnd": keep_bnd,
            "base": cont,
            "nedges": nedges,
            "crosses": crosses,
            "start": sp,
            "end": ep,
            "keep_pts": kp}
    # call
    c = com.contcom.PartitionContour(args)
    try:
        flow.exec_command(c)
        return c._get_added_names()[1][0]
    except Exception:
        raise ExecError('partition contour')


def matched_partition(cont, step, influence, ref_conts=[], ref_pts=[],
                      angle0=30, power=3):
    """ Makes a contour partition with respect to other
        contours partitions and given reference points

        :param cont: target contour.

        :param float step: default contour step size.

        :param float influence: influence radius of size conditions.

        :param ref_conts: list of contours which segmentation will
          be treated as target segmentation conditions.

        :param ref_pts: reference points given as
           ``[step0, [x0, y0], step1, [x1, y1], ...]`` list.

        :param float angle0: existing contour vertices which provide
           turns outside of ``[180 - angle0, 180 + angle0]`` degrees range
           will be preserved regardless of other options

        :param positive-float power: shows power of weight
           calculation function. As this parameter increases
           size transitions along contour become less smooth and more
           sensible to size conditions.

        See :ref:`matchedcontmeshing` for details.
    """
    if not isinstance(ref_conts, list):
        ref_conts = [ref_conts]

    errs = "invalid ref_pts parameter"
    if not isinstance(ref_pts, list) or len(ref_pts) % 2 != 0:
        raise ValueError(errs)
    for i in range(len(ref_pts) / 2):
        if not isinstance(ref_pts[2 * i], numbers.Real):
            raise ValueError(errs)
        if not isinstance(ref_pts[2 * i + 1], list) or\
                len(ref_pts[2 * i + 1]) != 2 or\
                not isinstance(ref_pts[2 * i + 1][0], numbers.Real) or\
                not isinstance(ref_pts[2 * i + 1][1], numbers.Real):
            raise ValueError(errs)
    rp = []
    for i in range(len(ref_pts) / 2):
        rp.append(ref_pts[2 * i])
        rp.extend(ref_pts[2 * i + 1])

    args = {"base": cont,
            "cconts": ref_conts,
            "cpts": rp,
            "step": step,
            "infdist": influence,
            "angle0": angle0,
            "power": power
            }
    c = com.contcom.MatchedPartition(args)
    try:
        flow.exec_command(c)
        return c._get_added_names()[1][0]
    except Exception:
        raise ExecError('matched partition')


def partition_segment(start, end, hstart, hend, hinternal=[]):
    """ Makes a partition of numeric segment by given
        recommended step sizes at different locations.

        :param float start:

        :param float end: start and end points of numeric segment.

        :param float hstart:

        :param float hend: recommended step at the beginning and at the
           end of numeric segments

        :param list-of-floats hinternal: possible internal recommended steps
           given as [segment value0, step0, segment value1, step1, ...]

        :returns: increasing float list representing
           partitioned segment [start, ..(internal steps).., end]

        :raises: ValueError, ExecError

        This is a helper function which runs iterative procedure
        to adopt partition of the segment to user defined step size
        recommendations. It could be used for example to
        explicitly define partition in :func:`add_unf_rect_grid`,
        :func:`add_unf_circ_grid` with given `custom\_\*` options or
        vertical partition of boundary grid in :func:`build_boundary_grid`.

        Example:
          Shows division of [1, 2] segment from approximately 0.1
          step size at the beginning to approximately 0.5 step size at
          the end

          .. code-block:: python

             hybmeshpack.hmscript.partition_segment(1, 2, 0.1, 0.5)
             >>> [1.0, 1.1238348, 1.309012, 1.585923, 2.0]

          Shows division of [1, 2] segment with approximate 0.1
          step sizes at its end points and 0.4 step size at the center.

          .. code-block:: python

             hybmeshpack.hmscript.partition_segment(1, 2, 0.1, 0.1, [1.5, 0.4])
             >>> [1.0, 1.1082238, 1.3278488, 1.672151, 1.891776, 2.0]


    """
    dch.ifnumericlist([start, end, hstart, hend] + hinternal)
    if len(hinternal) % 2 != 0:
        raise ValueError("hinternal should contain even number of entries")
    if (start >= end):
        raise ValueError("start number should be less then end number")
    for i in range(len(hinternal) / 2):
        if end <= hinternal[2 * i] or start >= hinternal[2 * i]:
            raise ValueError("inner point lies outside segment")
    try:
        ret = c2core.segment_part(start, end, hstart, hend, hinternal)
        return ret
    except Exception as e:
        raise ExecError(str(e))


def extract_subcontours(source, plist, project_to="vertex"):
    """ Extracts singly connected subcontours from given contour

        :param source: source contour identifier

        :param list-of-list-of-float plist: consecutive list of
           subcontours end points

        :param str project_to: defines projection rule for **plist** entries

           * ``"line"`` projects to closest point on the source contour
           * ``"vertex"`` projects to closest contour vertex
           * ``"corner"`` projects to closest contour corner vertex

        :returns: list of new contours identifiers

        :raises: ValueError, ExecError

        Length of **plist** should be equal or greater than two.
        First and last points in **plist** define first and last points
        of **source** segment to extract.
        All internal points define internal division
        of this segment. Hence number of resulting subcontours will equal
        number of points in **plist** minus one.

        For closed **source** contour first and last **plist** points
        could coincide. In that case the sum of resulting subcontours
        will be equal to **source**.
    """
    dch.ifpointlist(plist)
    args = {}
    args['src'] = source
    args['plist'] = [Point2(x[0], x[1]) for x in plist]
    args['project_to'] = project_to
    c = com.contcom.ExtractContour(args)
    try:
        flow.exec_command(c)
        return c._get_added_names()[1]
    except Exception:
        raise ExecError('extract subcontours')


def connect_subcontours(sources, fix=[], close="no", shiftnext=True):
    """ Connects sequence of open contours into a single contour
        even if neighboring contours have no equal end points

        :param sources: list of open contour identifiers

        :param list-of-int fix: indicies of **sources** contours
            which could not be shifted or stretched.

        :param str close: last connection algorithm:
            ``no``, ``yes`` or ``force``

        :param bool shiftnext: if True then each next contour will be
            shifted to the end point of previous one, otherwise
            both contours will be stretched to match average end point.

        To connect given contours this procedure implements stretching
        and shifting of those ones not listed in **fix** list.
        If two adjacent source contours are marked as fixed but have no
        common end points an exception will be raised.

        If **close** is ``yes`` then last contour of **sources** will
        be connected with the first one with algorithm depending on
        **fix** and **shiftnext** options.

        If **close** is ``no``
        then ending contours will be left as they are. In that case resulting
        contour will be open until first and last points are exactly equal.

        **close** = ``force`` algorithm works like **close** = ``no`` but
        creates a section which explicitly connects first and last contours
        by a straight line.
    """
    args = {}
    args['src'] = sources
    args['fix'] = copy.deepcopy(fix)
    args['close'] = close
    args['shiftnext'] = shiftnext
    c = com.contcom.ConnectSubcontours(args)
    try:
        flow.exec_command(c)
        return c._get_added_names()[1][0]
    except Exception:
        raise ExecError("connect subcontours")
