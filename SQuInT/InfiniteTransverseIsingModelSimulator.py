#@+leo-ver=4
#@+node:@file InfiniteTransverseIsingModelSimulator.py
"""Calculates the ground state energy of the transverse ising model."""

#import sys
#sys.stdout = open("log","w")

#@+others
#@+node:Import needed modules
#@+at
# Import the needed modules.  Just for kicks, we time this process since 
# sometimes it takes longer than the actual computation!
#@-at
#@@c
from timetracker import *
uber_timestamp = start_timing_event("Importing modules...")

from dequeue import dequeue

import numpy
from numpy import set_printoptions,array
import sys


#from numpy import multiply
from numpy.linalg import norm
from numpy.random import rand
from numpy import exp, log, imag, isfinite

import scipy
import scipy.linalg
from scipy.linalg.matfuncs import logm, expm

finished_timing_event(uber_timestamp,"done; %i seconds taken.")

try:
    #import psyco
    #print "Using Psyco!"
    #psyco.full()
    #psyco.profile()
    #psyco.log()
    pass
except ImportError:
    pass

from InfiniteOpenProductState import *

from time import time

#@+at
# import hotshot
# prof = hotshot.Profile("stones.prof")
# prof.start()
# 
#@-at
#@@c

from copy import copy, deepcopy

from scipy.linalg import svd, eigvals, schur

#@-node:Import needed modules
#@+node:Setup display options
#@+at
# Tell the system that it's okay to put more than 72 characters on a line...
#@-at
#@@c
set_printoptions(precision=4,linewidth=120)
#@nonl
#@-node:Setup display options
#@+node:Obtain parameters for run
#@+at
# Parameters are read from the command line;  if one or more are omitted, 
# hard-coded defaults are used instead.
# 
# First we setup a table with the names and types of parameters, as well as 
# their default values.
#@-at
#@@c

parameters = [
    ("lam","lambda",float,0.5),
    ("auxiliary_dimension","auxiliary dimension",int,5),
    ]


#@+at
# If the user does not specify any parameters, then it might be because he or 
# she does not know what the command-line
# arguments are.  Display a help message just in case.
#@-at
#@@c
print
if len(sys.argv) == 1:
    print "Usage:  %s %s" % (sys.argv[0],' '.join(map(lambda p: "[%s]"%p,zip(*parameters)[1])))
    print 
    print "When one or more of these parameters is omitted, defaults are employed in their stead."
    print
    print "(Even though no parameters were specified, this program will continue to run using the defaults.)"
    print

#@+at
# First parse whatever arguments are given by the user.
#@-at
#@@c
i = 1
while len(sys.argv) > i and i <= len(parameters):
    parameter_variable, parameter_name, parameter_type, default_value = parameters[i-1]
    parameter_value = parameter_type(sys.argv[i])
    print parameter_name,"=",parameter_value
    globals()[parameter_variable] = parameter_value
    i += 1

#@+at
# Now fill in the rest of the parameters with the defaults.
#@-at
#@@c
while i <= len(parameters):
    parameter_variable, parameter_name, parameter_type, default_value = parameters[i-1]
    print "Using default value",default_value,"for %s." % parameter_name
    globals()[parameter_variable] = default_value
    i += 1

#@-node:Obtain parameters for run
#@+node:Classes
#@+node:class ModelStack
class ModelStack(OneDimensionalSystem, dequeue):
    #@    @+others
    #@+node:__init__
    def __init__(self,*args,**keywords):
        OneDimensionalSystem.__init__(self,*args,**keywords)
        dequeue.__init__(self)
    #@-node:__init__
    #@-others


#@-node:class ModelStack
#@-node:Classes
#@+node:Initialize system
number_of_sites=10

timestamp = start_timing_event("Initializing configuration (including pre-normalization of sites array)...")

open_site_boundary = rand(auxiliary_dimension)
#open_site_boundary = array([1,]+[1,]*(auxiliary_dimension-1))

system = OneDimensionalSystem(
    site_dimension=2,
    auxiliary_dimension=auxiliary_dimension,
    initial_left_site_boundary=open_site_boundary,
    initial_right_site_boundary=open_site_boundary,
    )

finished_timing_event(timestamp,"done; %i seconds taken.")

#@+at
# Create the Hamiltonian.
#@-at
#@@c

Z = array([1,0,0,-1],complex128).reshape(2,2)
Y = array([0,1j,-1j, 0],complex128).reshape(2,2)
X = array([0,1,1, 0],complex128).reshape(2,2)
I = array([1,0,0, 1],complex128).reshape(2,2)


if False:
    #@    << Super Sparse operators >>
    #@+node:<< Super Sparse operators >>
    from sparse_tensor import sparse_tensor

    sparse_spin_coupling_left = sparse_tensor((3,),())
    sparse_spin_coupling_left[0] = 1

    sparse_spin_coupling_right = sparse_tensor((3,),())
    sparse_spin_coupling_right[2] = 1

    spin_coupling_operator_matrix = sparse_tensor((3,3),(2,2))
    spin_coupling_operator_matrix[0,0] =  I
    spin_coupling_operator_matrix[2,2] =  I
    spin_coupling_operator_matrix[0,1] =  X*(-lam)
    spin_coupling_operator_matrix[1,2] =  X
    spin_coupling_operator_matrix[0,2] = -Z
    spin_coupling_term = MatrixProductOperatorTermWithOpenBoundaryConditions(
        system,
        spin_coupling_operator_matrix,
        sparse_spin_coupling_left,
        sparse_spin_coupling_right,
        )

    #@-node:<< Super Sparse operators >>
    #@nl
elif False:
    #@    << Sparse operators >>
    #@+node:<< Sparse operators >>
    from sparse_tensor import sparse_tensor

    sparse_magnetic_left = sparse_tensor((2,),())
    sparse_magnetic_left[0] = 1

    sparse_magnetic_right = sparse_tensor((2,),())
    sparse_magnetic_right[1] = 1

    magnetic_field_operator_matrix = sparse_tensor((2,2,),(2,2))
    magnetic_field_operator_matrix[0,0] =  I
    magnetic_field_operator_matrix[1,1] =  I
    magnetic_field_operator_matrix[0,1] = -Z
    magnetic_field_term = MatrixProductOperatorTermWithOpenBoundaryConditions(
        system,
        magnetic_field_operator_matrix,
        sparse_magnetic_left,
        sparse_magnetic_right,
        )


    sparse_spin_coupling_left = sparse_tensor((3,),())
    sparse_spin_coupling_left[0] = 1

    sparse_spin_coupling_right = sparse_tensor((3,),())
    sparse_spin_coupling_right[2] = 1

    spin_coupling_operator_matrix = sparse_tensor((3,3),(2,2))
    spin_coupling_operator_matrix[0,0] =  I
    spin_coupling_operator_matrix[2,2] =  I
    spin_coupling_operator_matrix[0,1] =  X*(-lam)
    spin_coupling_operator_matrix[1,2] =  X
    spin_coupling_term = MatrixProductOperatorTermWithOpenBoundaryConditions(
        system,
        spin_coupling_operator_matrix,
        sparse_spin_coupling_left,
        sparse_spin_coupling_right,
        )


    #@-node:<< Sparse operators >>
    #@nl
else:
    #@    << Dense operators >>
    #@+node:<< Dense operators >>
    spin_coupling_operator_matrix = zeros((3,3,2,2),complex128)
    spin_coupling_operator_matrix[0,0] =  I
    spin_coupling_operator_matrix[2,2] =  I
    spin_coupling_operator_matrix[0,1] =  X*(-lam)
    spin_coupling_operator_matrix[1,2] =  X
    spin_coupling_operator_matrix[0,2] = -Z
    spin_coupling_term = MatrixProductOperatorTerm(
        system,
        spin_coupling_operator_matrix,
        array([1,0,0]),
        array([0,0,1])
        )
    #@-node:<< Dense operators >>
    #@nl

system.terms.append(spin_coupling_term)
#@-node:Initialize system
#@+node:Functions
#@+node:function infinite_energy_limit
def infinite_energy_limit(site_matrix):
    #@    @+others
    #@+node:Initialization
    A = site_matrix

    site_dimension = site_matrix.shape[0]
    auxiliary_dimension = site_matrix.shape[1]

    I = identity(2)

    #X = array([[0,1],[1,0]])
    #Z = array([[1,0],[0,-1]])
    #@-node:Initialization
    #@+node:Calculate left and right environment tensors
    S = site_matrix
    O = identity(site_dimension).reshape(1,1,site_dimension,site_dimension)

    def compute_environment(S,O):

        O_auxiliary_dimension = O.shape[0]

        retries = 0
        n = 0

        while n < 1e-10 and retries < 1:
            if retries > 0:
                print "REPEAT!!!"

            right_evals, right_evecs = my_eigen(
                lambda R: special_left_multiply(R.reshape(auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension),S,O).ravel(),
                O_auxiliary_dimension*auxiliary_dimension**2,k=4,ncv=11,which='LM')

            n = norm(right_evecs)

            #OO = compute_transfer_matrix(S,O).reshape((auxiliary_dimension**2,)*2)

            #print "REV:",right_evals
            #print [norm(right_evec) for right_evec in right_evecs.transpose()]

            #for eval in eigvals(OO):
            #    print eval

            #OO = compute_transfer_matrix(S,O).reshape((auxiliary_dimension,)*4).transpose(0,2,1,3).reshape((auxiliary_dimension**2,)*2)

            retries += 1

        retries = 0
        n = 0

        while n < 1e-10 and retries < 1:
            if retries > 0:
                print "REPEAT!!!"

            left_evals, left_evecs = my_eigen(
                lambda L: special_right_multiply(L.reshape(auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension),S,O).ravel(),
                O_auxiliary_dimension*auxiliary_dimension**2,k=4,ncv=11,which='LM')

            n = norm(left_evecs)
            retries += 1

            #print "LEV:",left_evals
            #print [norm(left_evec) for left_evec in left_evecs.transpose()]

        L = left_evecs[:,0].reshape((auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension))
        R = right_evecs[:,0].reshape((auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension))

        #print "EVALS:",left_evals,left_evecs.transpose(),dot(left_evecs[:,0],left_evecs[:,1])

        return L, R

    L, R = compute_environment(S,O)
    L = L.reshape((auxiliary_dimension,)*2)
    R = R.reshape((auxiliary_dimension,)*2)

    #compute_environment(S,magnetic_field_operator_matrix.to_array())
    #compute_environment(S,spin_coupling_operator_matrix.to_array())

    #@-node:Calculate left and right environment tensors
    #@+node:Compute Z term
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(-Z)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)
    g.connect(1,0,2,2)
    g.connect(1,1,4,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)

    Z_exp = s.merge_all().matrices[0]

    #print "Z_exp=",Z_exp

    #@-node:Compute Z term
    #@+node:Compute XX term
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(-lam*X)
    g.add_node(conj(A))

    g.add_node(A)
    g.add_node(X)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)

    g.connect(2,2,5,1)
    g.connect(4,2,7,1)
    g.connect(5,0,6,0)
    g.connect(7,0,6,1)

    g.connect(1,0,5,2)
    g.connect(1,1,7,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)
    s.add_node(5)
    s.add_node(6)
    s.add_node(7)

    XX_exp = s.merge_all().matrices[0]

    #print "XX_exp=",XX_exp

    #@-node:Compute XX term
    #@+node:Compute Z normalization
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)
    g.connect(1,0,2,2)
    g.connect(1,1,4,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)

    NZ_exp = s.merge_all().matrices[0]

    #print "NZ_exp=",NZ_exp

    #@-node:Compute Z normalization
    #@+node:Compute XX normalization
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)

    g.connect(2,2,5,1)
    g.connect(4,2,7,1)
    g.connect(5,0,6,0)
    g.connect(7,0,6,1)

    g.connect(1,0,5,2)
    g.connect(1,1,7,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)
    s.add_node(5)
    s.add_node(6)
    s.add_node(7)

    NXX_exp = s.merge_all().matrices[0]

    #print "NXX_exp=",NXX_exp

    #@-node:Compute XX normalization
    #@+node:Compute energy
    #print (Z_exp+XX_exp)/N_exp
    #print Z_exp/NZ_exp+XX_exp/NXX_exp

    E = Z_exp/NZ_exp+XX_exp/NXX_exp

    #assert abs(imag(E)) < 1e-10

    return E

    #print real(E), "%.2f" % (-log(abs(E-correct_answer))/log(10))

    #@-node:Compute energy
    #@-others
#@-node:function infinite_energy_limit
#@+node:my_eigen_schur
import arpack
import numpy as sb

def my_eigen_schur(matvec,n,k=2,ncv=5,
          maxiter=None,tol=0,guess=None,which='LM'):

    # some defaults
    if ncv is None:
        ncv=2*k+1
    ncv=min(ncv,n)
    if maxiter==None:
        maxiter=n*10

    typ = 'D'

    # some sanity checks
    if k <= 0:
        raise ValueError("k must be positive, k=%d"%k)
    if k >= n:
        raise ValueError("k must be less than rank(A), k=%d"%k)
    if maxiter <= 0:
        raise ValueError("maxiter must be positive, maxiter=%d"%maxiter)
    if ncv > n or ncv < k:
        raise ValueError("ncv must be k<=ncv<=n, ncv=%s"%ncv)

    eigsolver = arpack._arpack.znaupd
    eigextract = arpack._arpack.zneupd

    v = sb.zeros((n,ncv),typ) # holds Ritz vectors
    if guess is not None:
        guess = guess.copy().ravel()
        if guess.shape != (n,):
            raise ValueError, "guess has invalid dimensions [%s!=(%i,)]" % (guess.shape,n)
        resid = guess
        info = 1
    else:
        resid = sb.zeros(n,typ) # residual
        info = 0
    workd = sb.zeros(3*n,typ) # workspace
    workl = sb.zeros(3*ncv*ncv+6*ncv,typ) # workspace
    iparam = sb.zeros(11,'int') # problem parameters
    ipntr = sb.zeros(14,'int') # pointers into workspaces
    ido = 0

    rwork = sb.zeros(ncv,typ.lower())

    bmat = 'I'
    mode1 = 1

    ishfts = 1
    iparam[0] = ishfts
    iparam[2] = maxiter
    iparam[6] = mode1

    while ido != 99:

        ido,resid,v,iparam,ipntr,info =\
            eigsolver(ido,bmat,which,k,tol,resid,v,iparam,ipntr,
                      workd,workl,rwork,info)
        #if ido == 99:
        #    break
        #else:
        #source_slice      = slice(ipntr[0]-1, ipntr[0]-1+n)
        #destination_slice = slice(ipntr[1]-1, ipntr[1]-1+n)
        #workd[destination_slice]=matvec(workd[source_slice])
        workd[ipntr[1]-1:ipntr[1]-1+n] = matvec(workd[ipntr[0]-1:ipntr[0]-1+n])

    if info < -1 :
        raise RuntimeError("Error info=%d in arpack"%info)
        return None
    if info == -1:
        warnings.warn("Maximum number of iterations taken: %s"%iparam[2])

    # now extract eigenvalues and (optionally) eigenvectors        
    rvec = True #return_eigenvectors
    ierr = 0
    howmny = 'P' # return all eigenvectors
    sselect = sb.zeros(ncv,'int') # unused
    sigmai = 0
    sigmar = 0

    workev = sb.zeros(3*ncv,typ) 

    d,z,info =\
          eigextract(rvec,howmny,sselect,sigmar,workev,
                     bmat,which,k,tol,resid,v,iparam,ipntr,
                     workd,workl,rwork,ierr)   

    if ierr != 0:
        raise RuntimeError("Error info=%d in arpack"%info)
        return None

    my_eigen_schur.number_of_iterations = iparam[2]
    my_eigen_schur.resid = resid

    return d,z

#@-node:my_eigen_schur
#@+node:infinite_energy_limit

def infinite_energy_limit(site_matrix):

    auxiliary_dimension = system.auxiliary_dimension

    evals, evecs = my_eigen_schur(
        lambda R: special_left_multiply(R.reshape(auxiliary_dimension,auxiliary_dimension,3),site_matrix,spin_coupling_operator_matrix).ravel(),
        auxiliary_dimension**2 * 3,
        k=4,
        ncv=9,
        maxiter=1000,
        #tol=1e-13
        )

    sorted_indices = argsort(abs(evals))

    #print evals

    evecs = evecs[:,sorted_indices[-2:]]

    matrix = zeros((2,2),complex128)

    for i in xrange(2):
        for j in xrange(2):
            matrix[i,j] = dot(evecs[:,i].conj(),special_left_multiply(evecs[:,j].reshape(auxiliary_dimension,auxiliary_dimension,3),site_matrix,spin_coupling_operator_matrix).ravel())

    epsilon = sqrt(trace(dot(matrix,matrix.transpose().conj()))-2)



    new_right_evecs = [dot(evecs[:,i].reshape(auxiliary_dimension,auxiliary_dimension,1,3),array([0,0,1])) for i in [0,1]]
    new_left_evecs = [dot(evecs[:,i].reshape(auxiliary_dimension,auxiliary_dimension,1,3),array([1,0,0])) for i in [0,1]]

    #print matrix

    matrix = zeros((2,2),complex128)

    for i in xrange(2):
        for j in xrange(2):
            matrix[i,j] = dot(new_left_evecs[i].conj().ravel(),special_left_multiply(new_right_evecs[j],site_matrix,identity(2).reshape(1,1,2,2)).ravel())

    #print matrix

    epsilon_norm = sqrt(trace(dot(matrix,matrix.transpose().conj())))

    #print epsilon, epsilon_norm, epsilon/epsilon_norm

    return -real(epsilon/epsilon_norm)
#@-node:infinite_energy_limit
#@+node:function display_result
def display_result(final_A,residual=None):
    print
    print
    print "...and the peasants rejoiced."
    print

    limit_energy_timestamp = start_timing_event("Calculating final energy...")

    E = infinite_energy_limit(final_A)

    finished_timing_event(limit_energy_timestamp,"done!  Took %.2f second.")
    print
    if abs(imag(E)) > 1e-10:
        print "Energy:",E
    else:
        print "Energy:",real(E)

    correct_answers = {
        1.1:-1.34286402273,
        1.01:-1.27970376371,
        1:-1.273239544735,
        0.99:-1.266972193465,
        0.95:-1.2432657042699999,
        0.9:-1.2160009141099999,
        0.5:-1.063544409975,
        }

    if lam in correct_answers:
        correct_answer = correct_answers[lam]

        print "This agrees to %.2f digits with expected answer. (residual=%.3g)" % (-log(abs(E-correct_answer))/log(10),abs(E-correct_answer))

    time_taken = (time()-uber_timestamp)/60
    print 
    print "%.1f minutes taken so far;  average rate is %.2f minutes/site" % (time_taken,time_taken/total_number_of_sites)


#@-node:function display_result
#@+node:function infinite_energy_residual
def infinite_energy_residual(final_A):

    E = infinite_energy_limit(final_A)
    #print "ENERGY=",E
    if not isfinite(E):
        print E
        print "NOOOOOOOOOOOOOOOOOOOOOO!!!!"
    #    verbose_infinite_energy_limit(final_A)

    E = real(E)

    correct_answers = {
        1.1:-1.34286402273,
        1.01:-1.27970376371,
        1.001:-1.27387751469,
        1.0001:-1.27330322388,
        1:-1.273239544735,
        0.9999:-1.27317589993,
        0.999:-1.27260427634,
        0.99:-1.266972193465,
        0.95:-1.2432657042699999,
        0.9:-1.2160009141099999,
        0.5:-1.063544409975,
        }

    assert lam in correct_answers

    return abs(E-correct_answers[lam])
    #return -log(abs(E-correct_answer))/log(10)

#@-node:function infinite_energy_residual
#@+node:callback
def callback(iteration_number,number_of_iterations,current_A,diff,time_elapsed):
    global total_time_elapsed
    global total_time_elapsed_this_run

    if not iteration_number % 500 == 0 or diff > 1e8:
        total_time_elapsed += time_elapsed
        total_time_elapsed_this_run += time_elapsed
        return

    converge_time_elapsed = time_elapsed

    residual_timestamp = time()
    residual = infinite_energy_residual(current_A)
    residual_time_elapsed = time()-residual_timestamp

    time_elapsed = converge_time_elapsed+residual_time_elapsed

    total_time_elapsed += time_elapsed
    total_time_elapsed_this_run += time_elapsed
    string = "Iteration %i/%i -- %.2fs+%.1fs, %.1fm tot, %i sites/minute; " % (iteration_number,number_of_iterations,converge_time_elapsed,residual_time_elapsed,total_time_elapsed/60.0,iteration_number/total_time_elapsed_this_run*60)


    energy_digits = -log(residual)/log(10)
    string +=  "energy ~ **%.2f digits**; site ~ %.2f digits" % (energy_digits,-log(diff)/log(10))
    global last_energy_digits
    last_energy_digits = energy_digits
    #string += str(residual)
    print string

    stdout.flush()

    udiffs.append(diff)
    number_of_sites.append(system.total_number_of_sites)
    residuals.append(residual)

#@-node:callback
#@+node:xcallback
def xcallback(iteration_number,number_of_iterations,current_A,diff,time_elapsed):
    global total_time_elapsed
    global total_time_elapsed_this_run

    converge_time_elapsed = time_elapsed

    residual_timestamp = time()
    residual = infinite_energy_residual(current_A)
    residual_time_elapsed = time()-residual_timestamp

    time_elapsed = converge_time_elapsed+residual_time_elapsed

    total_time_elapsed += time_elapsed
    total_time_elapsed_this_run += time_elapsed
    string = "Iteration %i/%i -- %.2fs+%.1fs, %.1fm tot, %i sites/minute; " % (iteration_number,number_of_iterations,converge_time_elapsed,residual_time_elapsed,total_time_elapsed/60.0,iteration_number/total_time_elapsed_this_run*60)


    energy_digits = -log(residual)/log(10)
    string +=  "energy ~ **%.2f digits**; site ~ %.2f digits" % (energy_digits,-log(diff)/log(10))
    global last_energy_digits
    last_energy_digits = energy_digits
    #string += str(residual)
    print string

    stdout.flush()

    udiffs.append(diff)
    number_of_sites.append(system.total_number_of_sites)
    residuals.append(residual)

#@-node:xcallback
#@-node:Functions
#@+node:Do run
number_of_sites = []
residuals = []
udiffs = []
times = []

last_energy_digits = 0

total_time_elapsed_this_run = 0
total_time_elapsed = time()-uber_timestamp

from sys import stdout

try:
    for i in xrange(4):
        system.repeat_alternating_site_run(
            number_of_iterations=20,
            tol=1e-8,
            callback=xcallback,
            number_of_times_to_absorb=1,
            energy_raise_threshold=1e-6,
            promise_keeper_threshold=None,
        )
        system.repeat_alternating_site_run(
            number_of_iterations=2000,
            tol=1e-8,
            callback=callback,
            number_of_times_to_absorb=1,
        )

        tstamp = start_timing_event("Increasing auxiliary dimension to %i..." % (system.auxiliary_dimension+5))
        system.increase_auxiliary_dimension_by(5)
        finished_timing_event(tstamp,"done!  Took %.1f seconds.")
except KeyboardInterrupt:
    pass
#@-node:Do run
#@-others

#@-node:@file InfiniteTransverseIsingModelSimulator.py
#@-leo
