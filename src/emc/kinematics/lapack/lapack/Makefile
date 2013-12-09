SHELL = /bin/sh

FORTRAN  = gfortran
OPTS     = -O2 -frecursive
DRVOPTS  = $(OPTS)
NOOPT    = -O0 -frecursive
LOADER   = gfortran
LOADOPTS =

TIMER    = INT_ETIME

EXTRA_CFLAGS += -I. -I/usr/realtime-2.6.32-122-rtai/include -I. -I/usr/realtime-2.6.32-122-rtai/include -D_FORTIFY_SOURCE=0 -mhard-float -DRTAI=3 -DRTAPI -D_GNU_SOURCE -Drealtime -D_FORTIFY_SOURCE=2 -D__MODULE__ -I/home/frankt/git/ikfast/src -I/home/frankt/git/ikfast/src/libnml/linklist -I/home/frankt/git/ikfast/src/libnml/cms -I/home/frankt/git/ikfast/src/libnml/rcs -I/home/frankt/git/ikfast/src/libnml/inifile -I/home/frankt/git/ikfast/src/libnml/os_intf -I/home/frankt/git/ikfast/src/libnml/nml -I/home/frankt/git/ikfast/src/libnml/buffer -I/home/frankt/git/ikfast/src/libnml/posemath -I/home/frankt/git/ikfast/src/rtapi -I/home/frankt/git/ikfast/src/hal -I/home/frankt/git/ikfast/src/emc/nml_intf -I/home/frankt/git/ikfast/src/emc/kinematics -I/home/frankt/git/ikfast/src/emc/motion -DSEQUENTIAL_SUPPORT -DHAL_SUPPORT -DDYNAMIC_PLCSIZE -DRT_SUPPORT -DOLD_TIMERS_MONOS_SUPPORT -DMODBUS_IO_MASTER -fno-fast-math -mieee-fp -fno-unsafe-math-optimizations -Wframe-larger-than=2560

CC = gcc
CFLAGS = -O3 $(EXTRA_CFLAGS)

XBLASLIB     =

ARCH     = ar
ARCHFLAGS= cr
RANLIB   = ranlib


DOBJS= double/dgebak.o double/dgebal.o double/dgeev.o double/dgehd2.o double/dgehrd.o double/dgetf2.o double/dgetrf.o double/dgetrs.o double/dhseqr.o double/disnan.o double/dlabad.o double/dlacpy.o double/dladiv.o double/dlaexc.o double/dlahqr.o double/dlahr2.o double/dlaisnan.o double/dlaln2.o double/dlange.o double/dlanv2.o double/dlapy2.o double/dlaqr0.o double/dlaqr1.o double/dlaqr2.o double/dlaqr3.o double/dlaqr4.o double/dlaqr5.o double/dlarfb.o double/dlarf.o double/dlarfg.o double/dlarft.o double/dlarfx.o double/dlartg.o double/dlascl.o double/dlaset.o double/dlassq.o double/dlaswp.o double/dlasy2.o double/dorg2r.o double/dorghr.o double/dorgqr.o double/dorm2r.o double/dormhr.o double/dormqr.o double/dtrevc.o double/dtrexc.o
LOBJS= lapack_routine/dlamch.o lapack_routine/ieeeck.o lapack_routine/iladlc.o lapack_routine/iladlr.o lapack_routine/ilaenv.o lapack_routine/iparmq.o lapack_routine/lsame.o lapack_routine/xerbla.o
BOBJS= blas/daxpy.o blas/dcopy.o blas/ddot.o blas/dgemv.o blas/dger.o blas/dnrm2.o blas/drot.o blas/dscal.o blas/dswap.o blas/dtrmm.o blas/dtrmv.o blas/dtrsm.o blas/idamax.o blas/dgemm.o
GOBJS=gfortranlib/stop.o gfortranlib/string_intrinsics.o gfortranlib/others.o

FOBJS=$(DOBJS) $(LOBJS) $(BOBJS)
ALLOBJS=$(FOBJS) $(GOBJS)

FSRC=$(FOBJS:.o=.f)

%.o: %.f 
	$(FORTRAN) -c $< -o $@

../../liblapack.a:	$(ALLOBJS) 
	$(ARCH) $(ARCHFLAGS) $@ $(ALLOBJS)
	$(RANLIB) $@

