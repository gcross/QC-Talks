#@+leo-ver=4
#@+node:@file InfiniteOpenProductState.py
#@+others
#@+node:Import needed modules
from numpy import *
from scipy.linalg.iterative import *
from numpy.random import rand
from scipy.linalg import *
import scipy.linalg
from scipy.linalg.basic import LinAlgError
from numpy.linalg import inv
import sys

import warnings

from Graph import Graph, Subgraph, Placeholder, compile_graph

from hooks import outer_product

import arpack
import numpy as sb

try:
    from arpack import eigen
except ImportError:
    pass

from timetracker import *
#@nonl
#@-node:Import needed modules
#@+node:Constants
LEFT  = -1
RIGHT = +1

UP = 1j
DOWN = -1j
#@-node:Constants
#@+node:Functions
#@+node:Normalization functions
#@+node:function compute_normalizer
def compute_normalizer(B,index):
    """Computes X such that sum_{s'=s} (BX)^dagger (BX) = I."""

    tensordot_indices = range(B.ndim)
    del tensordot_indices[index]

    M = tensordot(conjugate(B),B,(tensordot_indices,)*2)

    # $\bM^{[i]} = \sum_s \bB^{[i],s\,\,\dagger}\cdot\bB^{[i],s}$;  the \cs{trace} method is what sums over s=s'
    # $\bM \vx^{(i)} = \lambda \vx^{(i)}$;  $U_{ji} = x^{(i)}_j$
    vals, U = scipy.linalg.eigh(M)

    # Replace near-zeros and negative numbers with zero;  note that since $\bM$ is completely positive
    # and Hermitian all eigenvalues should be positive, so any negative eigenvalues should just be
    # small approximations to zero.
    vals[vals<0] = 0

    # $\bX = \bU \bD^{-1/2}$;  note that the pseudo-inverse is used since some eigenvalues might be zero
    dvals = sqrt(vals)
    nonzero_dvals = dvals!=0    
    dvals[nonzero_dvals] = 1/dvals[nonzero_dvals]
    D = diag(dvals)
    X = dot(dot(U,D),U.conj().transpose())

    return X
#@-node:function compute_normalizer
#@+node:function apply_normalizer
def apply_normalizer(normalizer,matrix,index):
    matrix_new_indices = range(matrix.ndim-1)
    matrix_new_indices.insert(index,matrix.ndim-1)
    return tensordot(matrix,normalizer,(index,0)).transpose(matrix_new_indices)

#@+at
# def apply_inverse_normalizer(normalizer,matrix,index):
#     matrix_new_indices = range(matrix.ndim-1)
#     matrix_new_indices.insert(index,matrix.ndim-1)
#     try:
#         normalizer_inverse = inv(normalizer)
#     except:
#         raise
#         normalizer_inverse = pinv(normalizer)
#     return 
# tensordot(matrix,normalizer_inverse,(index,1)).transpose(matrix_new_indices)
#@-at
#@@c
#@-node:function apply_normalizer
#@+node:function compute_and_apply_normalizer
def compute_and_apply_normalizer(matrix,index):
    return apply_normalizer(compute_normalizer(matrix,index),matrix,index)
#@-node:function compute_and_apply_normalizer
#@-node:Normalization functions
#@+node:my_eigen
def my_eigen(matvec,n,k=1,ncv=None,
          maxiter=None,tol=0,guess=None,which='SR'):
    if n == 1:
        return matvec(array([1])), array([[1]])

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
    howmny = 'A' # return all eigenvectors
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

    my_eigen.number_of_iterations = iparam[2]
    my_eigen.resid = resid

    return d,z

#@-node:my_eigen
#@+node:normalize
def normalize(matrix,index):
    new_indices = range(matrix.ndim)
    del new_indices[index]
    new_indices.append(index)

    old_shape = list(matrix.shape)
    del old_shape[index]
    new_shape = (prod(old_shape),matrix.shape[index])
    old_shape.append(matrix.shape[index])

    new_matrix = matrix.transpose(new_indices).reshape(new_shape)

    old_indices = range(matrix.ndim-1)
    old_indices.insert(index,matrix.ndim-1)

    try:
        u, s, v = svd(new_matrix,full_matrices=0)    
        return dot(u,v).reshape(old_shape).transpose(old_indices)
    except LinAlgError:
        M = dot(new_matrix.conj().transpose(),new_matrix)

        vals, U = eigh(M)
        vals[vals<0] = 0

        dvals = sqrt(vals)
        nonzero_dvals = dvals!=0
        dvals[nonzero_dvals] = 1.0/dvals[nonzero_dvals]
        X = dot(U*dvals,U.conj().transpose())

        return dot(new_matrix,X).reshape(old_shape).transpose(old_indices)

#@-node:normalize
#@+node:compute_optimized_site_matrix
# \ESC{\subsection{optimize} \index{optimize}}
def compute_optimized_site_matrix(n,matvec,compute_energy,old_energy=None,guess=None,iteration_cap=None,tol=0,energy_raise_threshold=1e-10,k=1,ncv=3):
    """Adjusts the matrix at the site given by *site_number* so that the energy of the system is minimized.  If *next_site* is specified as either LEFT or RIGHT, then it will normalize the site by respectively left- or right- multiplying the site by an operator and then it will take the adjacent site and multiply the opposite side by the inverse of this operator in order to preserve the state."""
    #from time import time

    #uber_timestamp = time()

    #ncv = min(5,n)
    #ncv = min(3,n)
    ncv = min(ncv,n)
    try:
        #timestamp = time()
        eigenvalues, eigenvectors = my_eigen(matvec,n,k=k,ncv=ncv,guess=guess,maxiter=iteration_cap,tol=tol)
        #print (time()-timestamp)/my_eigen.number_of_iterations
    except ValueError, msg: # thrown when the eigenvalue solver does not converge to a solution
        # Signal to the caller that there was a problem.
        raise
        raise ConvergenceError, "Problem converging to solution in eigenvalue solver: "+str(msg)

    eigenvectors = eigenvectors.transpose()

#@+at
# It's not enough to simply take the eigenvector corresponding to the lowest 
# eigenvalue as our solution, since sometimes for various reasons (due mainly 
# to numerical instability) some or all of the eigenvalues are bogus.  For 
# example, sometimes the algorithm returns th the equivalent of $-\infty$, 
# which generally corresponds to an eigenvector that is very nearly zero.  
# Thus, it is important to filter out all of these bogus solutions before 
# declaring victory.
# 
#@-at
#@@c
    # Sort the eigenvalues.
    sorted_indices_of_eigenvalues = argsort(real(eigenvalues))

#@+at
# A solution $\lambda,\vx$ is acceptable iff:
# 
# \begin{itemize}
# \item $\|\vx\|_\infty > 1\cdot 10^{-10}$, to rule out erroneous ``nearly 
# zero'' solutions that result in infinitely large eigenvalues
# \item $\abs{\text{Im}[\lambda]} < 1\cdot 10^{-10}$, since energies should be 
# \emph{real} as the Hamiltonian is hermitian.  (Small imaginary parts aren't 
# great, but they don't hurt too much as long as they are sufficiently 
# negligible.)
# \item $\abs{x_i}<\infty, \abs{\lambda} < \infty$
# \item $\abs{\frac{\coip{\vx}{\bD}{\vx}}{\coip{\vx}{\bN}{\vx}} - \lam}<1\cdot 
# 10^{-10}$, since sometimes the returned eigenvalue is actually much 
# different from the energy obtained by using the corresponding eigenvector
# \end{itemize}
#@-at
#@@c
    def is_acceptable(index):
        evec = eigenvectors[index]
        eval = eigenvalues[index]
        return max(abs(evec)) > 1e-10 and isfinite(evec).all() and isfinite(eval)

    acceptable_indices_of_eigenvalues = filter(is_acceptable,sorted_indices_of_eigenvalues)
    if len(acceptable_indices_of_eigenvalues) == 0:
        print eigenvalues
        print eigenvectors
        raise ConvergenceError, "All eigenvectors had near-vanishing normals, eigenvalues with large imaginary parts, or NaNs and/or infs."

#@+at
# If we've reached this point, then we have at least one solution that isn't 
# atrocious.  However, it's possible that our solution has a higher energy 
# than our current state -- either due to a problem in the eigenvalue solver, 
# or because we filtered out all the lower-energy solutions.  Thus, we need 
# compare the energy of the solution we've found to our old energy to make 
# sure that it's higher.
#@-at
#@@c    
    index_of_solution = acceptable_indices_of_eigenvalues[0]
    solution = eigenvectors[index_of_solution]

    #new_energy = 0
    #for L, O, R in terms:
    #    new_energy += dot(solution.conj(),special_matvec(L,O,R,A).ravel())

    #new_energy = compute_energy(solution)#dot(solution.conj(),matvec(solution))

    new_energy = eigenvalues[index_of_solution]

    new_energy = real(new_energy)

    if old_energy is not None and real(new_energy) > real(old_energy) and abs(new_energy-old_energy)>energy_raise_threshold and False:
        raise ConvergenceError, "Only found a solution which *raised* the energy. (%.15f < %.15f)" % (new_energy,old_energy)

    #print "time per site = ", (time()-uber_timestamp)/my_eigen.number_of_iterations

    return solution, new_energy
#@-node:compute_optimized_site_matrix
#@+node:function kill_phase
def kill_phase(x):
    x_ravel = x.ravel()
    index = argmax(abs(x_ravel))
    x *= (conj(x_ravel[index])/abs(x_ravel[index]))
#@-node:function kill_phase
#@+node:multiply_tensor_by_matrix_at_index
def multiply_tensor_by_matrix_at_index(tensor,matrix,index):
    tensor_new_indices = range(tensor.ndim-1)
    tensor_new_indices.insert(index,tensor.ndim-1)
    return tensordot(tensor,matrix,(index,0)).transpose(tensor_new_indices)
#@nonl
#@-node:multiply_tensor_by_matrix_at_index
#@+node:ensure_dimensions_greater_than_one
def ensure_dimensions_greater_than_one(shape):
    return tuple(x if x!=1 else 2 for x in shape)
#@-node:ensure_dimensions_greater_than_one
#@-node:Functions
#@+node:Classes
#@+node:class ConvergenceError
class ConvergenceError(Exception):
    pass
#@-node:class ConvergenceError
#@+node:class System
class System:
    #@    @+others
    #@+node:__init__
    def __init__(self):
        self.total_number_of_sites = 0

        self.terms = []
    #@-node:__init__
    #@+node:absorb_site
    def absorb_site(self,direction,number_of_times_to_absorb=1):

        for i in xrange(number_of_times_to_absorb):
            for term in self.terms + [self.normalization_term]:
                term.absorb_site(direction)

        self.total_number_of_sites += 1


    #@-node:absorb_site
    #@+node:repeat_alternating_site_run
    def repeat_alternating_site_run(self,number_of_iterations,terminate_if_within=0,tol=0,callback=None,number_of_times_to_absorb=1,energy_raise_threshold=1e-10, promise_keeper_threshold=1e-2):

        iterations_so_far = 1
        direction = LEFT
        while iterations_so_far <= number_of_iterations:
            last_unnormalized_site_matrix = self.unnormalized_site_matrix
            self.single_site_run(
                direction,
                tol=tol,
                energy_raise_threshold=energy_raise_threshold,
                promise_keeper_threshold=promise_keeper_threshold,
                number_of_times_to_absorb=number_of_times_to_absorb,
                )
            if last_unnormalized_site_matrix is not None:
                udiff =  norm(last_unnormalized_site_matrix-self.unnormalized_site_matrix)/max(norm(self.unnormalized_site_matrix),norm(last_unnormalized_site_matrix))
                if callback:
                    callback(iterations_so_far,number_of_iterations,self.site_matrix,udiff,self.time_elapsed)
                if udiff < terminate_if_within:
                    break

            iterations_so_far += 1
            direction *= -1
    #@-node:repeat_alternating_site_run
    #@+node:single_site_run
    def single_site_run(self, direction_to_absorb, tol=0, number_of_times_to_absorb = 1, energy_raise_threshold = 1e-10, promise_keeper_threshold = 1e-2):
        timestamp = time()
        old_energy = self.compute_energy()
        site_matrix, promised_energy = compute_optimized_site_matrix(self.site_matrix_flattened_dimension,self.get_matvec(),self.compute_energy_given_site_matrix,old_energy,guess=self.unnormalized_site_matrix,tol=tol,energy_raise_threshold=energy_raise_threshold)
        self.site_matrix = site_matrix.reshape(self.shape_of_site_matrix)
        self.unnormalized_site_matrix = self.site_matrix.copy()
        kill_phase(self.unnormalized_site_matrix)
        calculated_energy = self.compute_energy()
        if promise_keeper_threshold is not None:
            difference_between_calculated_and_promised_energy = abs(calculated_energy-promised_energy)/abs(promised_energy)
            if difference_between_calculated_and_promised_energy>promise_keeper_threshold and abs(calculated_energy)+abs(promised_energy)>1e-10: #1e-8: # and abs(promised_energy) > 1e-10
                raise ConvergenceError, "Calculated energy for system (%g) was different from promised energy (%g) by %g%%." % (calculated_energy,promised_energy,difference_between_calculated_and_promised_energy*100)
        if not isfinite(self.compute_energy()):
            raise ConvergenceError, "New state has non-finite answer for energy."
        self.normalize_and_absorb_site(direction_to_absorb)#,number_of_times_to_absorb=number_of_times_to_absorb)
        self.total_number_of_sites += 1
        self.time_elapsed = time()-timestamp
    #@-node:single_site_run
    #@+node:compute_energy_given_site_matrix
    def compute_energy_given_site_matrix(self,site_matrix):
        site_matrix = site_matrix.reshape(self.shape_of_site_matrix)
        return sum([dot(site_matrix.ravel().conj(),term.get_matvec()(site_matrix).ravel()) for term in self.terms])/dot(site_matrix.ravel().conj(),self.normalization_term.get_matvec()(site_matrix).ravel())

    #@-node:compute_energy_given_site_matrix
    #@+node:get_matvec
    def get_matvec(self):

        norm_imatvec = self.normalization_term.get_imatvec()

        matvec_funcs = [x for x in (term.get_matvec() for term in self.terms) if x is not None]

        number_of_matvec_funcs = len(matvec_funcs)

        shape_of_site_matrix = self.shape_of_site_matrix

        def matvec(A):
            A = A.reshape(shape_of_site_matrix)
            result = matvec_funcs[0](A)
            i = 1
            while i < number_of_matvec_funcs:
                result += matvec_funcs[i](A)
                i += 1

            return norm_imatvec(result).ravel()

        return matvec
    #@-node:get_matvec
    #@+node:compute_energy
    # \ESC{\subsection{compute\_energy}}
    def compute_energy(self):
        """Computes the energy of the system in its current state."""

        # The energy of the system is defined to be
        #
        # $$\exp{E} := \frac{\coip{\psi}{\bH}{\psi}}{\cip{\psi}{\psi}}.$$
        #
        # Note that, as discussed in section \ref{expectation-matrices}, expectations are computed via. computing the
        # trace of the product of expectation matrices -- that is,
        #
        # $\exp{\bO_1\otimes\dots\bO_N} \equiv \tr[\bE_{\bO_1}\cdot\dots\cdot \bE_{\bO_N}].$
        return sum(map(lambda term: term.get_expectation_value(), self.terms)) / self.normalization_term.get_expectation_value()

        energy1 = sum(map(lambda term: term.get_expectation_value(), self.terms)) / self.normalization_term.get_expectation_value()

        energy2 = dot(self.site_matrix.ravel().conj(),self.get_matvec()(self.site_matrix.ravel()))

        if abs(energy1-energy2)>1e-10:
            print abs(energy1-energy2)/abs(energy1+energy2)
            raise RuntimeError
        else:
            print "YAY!"

        return energy1
    #@-node:compute_energy
    #@-others
#@-node:class System
#@-node:Classes
#@+node:One Dimension
#@+node:Special multiplication functions
g = Graph()

#@+at
# Note:
# 
# These dimensions are not the actual dimensions;  they are random prime 
# numbers.
# 
# This is done so that index checking can be performed (i.e., to make sure 
# that I've
# connected all of the indices properly).  The function that is produced at 
# the end
# of the day will not depend on these dimensions at all.
# 
#@-at
#@@c

L = Placeholder("L",(101,101,3))
A = Placeholder("A",(2,101,111))
O = Placeholder("O",(3,3,2,2))
R = Placeholder("R",(111,111,3))

g.add_node(L)        #L_cc'c''
g.add_node(A)        #A_sab
g.add_node(O)        #O_a''b''ss'
g.add_node(A.conj()) #A*_s'a'b'
g.add_node(R)        #R_dd'd''

g.connect(1,1,0,0) #A_sab <--a=c--> L_cc'c''
g.connect(1,2,4,0) #A_sab <--b=d--> R_dd'd''
g.connect(1,0,2,2) #A_sab <--s=s--> O_a''b''ss'
g.connect(3,1,0,1) #A*_sab <--a'=c'--> L_cc'c''
g.connect(3,2,4,1) #A*_sab <--b'=d'--> R_dd'd''
g.connect(3,0,2,3) #A*_sab <--s'=s'--> O_a''b''ss'
g.connect(2,0,0,2) #O_a''b''ss' <--a''=c''--> L_cc'c''
g.connect(2,1,4,2) #O_a''b''ss' <--b''=d''--> R_dd'd''

L_data = rand(*L.shape)
A_data = rand(*A.shape)+1j*rand(*A.shape)
O_data = rand(*O.shape)
R_data = rand(*R.shape)

special_right_multiply = compile_graph(g,[0,1,2,3],["L","A","O"],function_name="special_right_multiply")
special_left_multiply = compile_graph(g,[1,2,3,4],["R","A","O"],function_name="special_left_multiply")
compute_transfer_matrix = compile_graph(g,[1,2,3],["A","O"],node_ordering=[0,4],function_name="compute_transfer_matrix")
compute_opt_matrix = compile_graph(g,[0,2,4],["L","O","R"],node_ordering=[3,1],function_name="compute_optimization_matrix")

special_matvec = compile_graph(g,[0,1,2,4],["L","O","R","A"],function_name="special_matvec")
special_rmatvec = compile_graph(g,[0,2,3,4],["L","O","R","A"],function_name="special_rmatvec")

s = Subgraph(g)
for i in xrange(5):
    s.add_node(i)

ma = s.merge_all()
special_expectation_value = s.merge_all().matrices[0].compile_with_name("special_expectation_value","L","O","R","A")

#print merge_graph(g,[0,1,2,4])

special_multiplication_graph = g
del s, g, i


g = Graph()
g.add_node(Placeholder("L",(101,101,5)))
g.add_node(Placeholder("N",(101,101)))
g.add_node(Placeholder("R1",(101,)))
g.add_node(Placeholder("R2",(101,)))
g.add_node(Placeholder("R3",(5,)))

g.connect(0,0,2,0)
g.connect(0,1,1,1)
g.connect(0,2,4,0)
g.connect(1,0,3,0)

special_multiply_inverse = compile_graph(g,[0,1],["L","N"],node_ordering=[2,3,4],function_name="special_multiply_inverse")

del g



g = Graph()
g.add_node(Placeholder("IN",(101,101,5)))
g.add_node(Placeholder("OUT",(77,77,5)))
g.add_node(Placeholder("UH",(101,77)))
g.add_node(Placeholder("UHD",(101,77)))

g.connect(0,0,2,0)
g.connect(0,1,3,0)
g.connect(1,0,2,1)
g.connect(1,1,3,1)
g.connect(0,2,1,2)

special_multiply_by_normalizer = compile_graph(g,[0,2,3],["IN","UH","UHD"],function_name="special_multiply_by_normalizer")

del g





g = Graph()
g.add_node(Placeholder("IN",(2,77,77)))
g.add_node(Placeholder("OUT",(2,101,101)))
g.add_node(Placeholder("UH",(101,77)))
g.add_node(Placeholder("VH",(101,77)))

g.connect(0,1,2,1)
g.connect(0,2,3,1)
g.connect(1,1,2,0)
g.connect(1,2,3,0)
g.connect(0,0,1,0)

special_rotate_by_normalizer = compile_graph(g,[0,2,3],["IN","UH","VH"],function_name="special_rotate_by_normalizer")

del g

del L,A,O,R
#@-node:Special multiplication functions
#@+node:class OneDimensionalSystem
class OneDimensionalSystem(System):
    #@    @+others
    #@+node:__init__
    def __init__(self,site_dimension,auxiliary_dimension,initial_left_site_boundary,initial_right_site_boundary,create_normalization_term=True):
        System.__init__(self)
        self.site_dimension = site_dimension
        self.auxiliary_dimension = auxiliary_dimension
        self.initial_left_site_boundary = initial_left_site_boundary
        self.initial_right_site_boundary = initial_right_site_boundary

        self.update_quantities_derivative_from_auxiliary_dimension()

        self.site_matrix = 1-2*rand(site_dimension,auxiliary_dimension,auxiliary_dimension)+1j-2j*rand(site_dimension,auxiliary_dimension,auxiliary_dimension)
        self.unnormalized_site_matrix = None

        if create_normalization_term:
            self.normalization_term = MatrixProductOperatorTerm(self,identity(self.site_dimension).reshape(1,1,self.site_dimension,self.site_dimension),ones((1)),ones((1)))


    #@-node:__init__
    #@+node:normalize_and_absorb_site
    def normalize_and_absorb_site(self,direction,number_of_times_to_absorb=1):
        if direction == LEFT:
            self.site_matrix = normalize(self.site_matrix,2)
        elif direction == RIGHT:
            self.site_matrix = normalize(self.site_matrix,1)
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

        self.absorb_site(direction,number_of_times_to_absorb=number_of_times_to_absorb)
    #@-node:normalize_and_absorb_site
    #@+node:increase_auxiliary_dimension_by
    def increase_auxiliary_dimension_by(self,increment):
        u, s, v = svd(2*rand(self.auxiliary_dimension,self.auxiliary_dimension+increment)-1+2j*rand(self.auxiliary_dimension,self.auxiliary_dimension+increment)-1j)

        s = diag(ones(v.shape[0]))[:u.shape[0]]

        unitary = reduce(dot,[u,s,v])
        for term in [self.normalization_term] + self.terms:
            term.absorb_unitary(unitary)

        self.auxiliary_dimension += increment
        self.update_quantities_derivative_from_auxiliary_dimension()

        if self.site_matrix is not None:
            self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary.conj(),1)
            self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary,2)

        if self.unnormalized_site_matrix is not None:
            self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary.conj(),1)
            self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary,2)

        return unitary


    #@+at
    #     if self.site_matrix is not None:
    #         self.site_matrix = 
    # reduce(dot,[unitary.conj().transpose(),self.site_matrix,unitary]).transpose(1,0,2)
    # 
    #     if self.unnormalized_site_matrix is not None:
    #         self.unnormalized_site_matrix = 
    # reduce(dot,[unitary.conj().transpose(),self.unnormalized_site_matrix,unitary]).transpose(1,0,2)
    # 
    #@-at
    #@@c


    #@-node:increase_auxiliary_dimension_by
    #@+node:update_quantities_derivative_from_auxiliary_dimension
    def update_quantities_derivative_from_auxiliary_dimension(self):
        self.site_matrix_flattened_dimension = self.site_dimension*self.auxiliary_dimension**2
        self.shape_of_site_matrix = (self.site_dimension,self.auxiliary_dimension,self.auxiliary_dimension)

    #@-node:update_quantities_derivative_from_auxiliary_dimension
    #@-others
#@-node:class OneDimensionalSystem
#@+node:Terms
#@+node:class MatrixProductOperatorTerm
class MatrixProductOperatorTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,operator_matrix,left_operator_boundary,right_operator_boundary):
        self.system = system
        self.operator_matrix = operator_matrix

        self.left_operator_boundary = left_operator_boundary
        self.right_operator_boundary = right_operator_boundary

        self.left_boundary = reduce(outer_product,[system.initial_left_site_boundary,conj(system.initial_left_site_boundary),self.left_operator_boundary])
        self.right_boundary = reduce(outer_product,[system.initial_right_site_boundary,conj(system.initial_right_site_boundary),self.right_operator_boundary])

    #@-node:__init__
    #@+node:get_matvec
    def get_matvec(self):
        L = self.left_boundary
        O = self.operator_matrix
        R = self.right_boundary
        return lambda A: special_matvec(L,O,R,A) 
    #@-node:get_matvec
    #@+node:get_imatvec
    def get_imatvec(self):

    #@+at
    #     class Dummy:
    #         pass
    # 
    #     d = Dummy()
    # 
    #     shape_of_site = 
    # (self.system.site_dimension,self.system.auxiliary_dimension,self.system.auxiliary_dimension)
    # 
    #     matvec = self.get_matvec()
    #     d.matvec = lambda A: matvec(A.reshape(shape_of_site)).ravel()
    # 
    #     #rmatvec = self.get_rmatvec()
    #     #d.rmatvec = lambda A: rmatvec(A.reshape(shape_of_site)).ravel()
    # 
    #     def imatvec(vector):
    #         x, info = bicgstab(d,vector.ravel(),tol=10e-10)
    #         assert info == 0
    #         return x.reshape(shape_of_site)
    # 
    #     return imatvec
    #@-at
    #@@c


        if self.operator_matrix.shape[0] > 1:
            optimization_matrix = self.get_optimization_matrix()

            inverse_optimization_matrix = pinv2(optimization_matrix)

            return lambda vector:  dot(inverse_optimization_matrix,vector.ravel()).reshape(vector.shape)

        else:

            norm_L = self.left_boundary
            norm_R = self.right_boundary

            norm_L = norm_L.reshape(norm_L.shape[:-1])
            norm_R = norm_R.reshape(norm_R.shape[:-1])

            def compute_inverse(M):
                evals, evecs = eigh(M) 
                evecs = evecs.transpose()

                evals[abs(evals)<1e-13] = 0
                evals[abs(evals)>=1e-13] = 1/evals[abs(evals)>=1e-13]

                return dot(evecs.conj().transpose(),dot(diag(evals),evecs))

            inverse_norm_L = compute_inverse(norm_L.transpose())
            inverse_norm_R = compute_inverse(norm_R.transpose())

            return lambda site_matrix: multiply_tensor_by_matrix_at_index(multiply_tensor_by_matrix_at_index(site_matrix,inverse_norm_L,1),inverse_norm_R,2)


    #@-node:get_imatvec
    #@+node:get_rmatvec
    def get_rmatvec(self):
        L = self.left_boundary
        O = self.operator_matrix
        R = self.right_boundary
        return lambda A: special_rmatvec(L,O,R,A) 
    #@-node:get_rmatvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        return special_expectation_value(
            self.left_boundary,
            self.operator_matrix,
            self.right_boundary,
            self.system.site_matrix,
        )

    #@+at    
    #     return  tensordot(
    #         self.left_boundary,
    #         special_left_multiply(
    #             self.right_boundary,
    #             self.system.site_matrix,
    #             self.operator_matrix
    #         ),
    #         ((0,1,2),(0,1,2))
    #     )
    #@-at
    #@@c


    #@-node:get_expectation_value
    #@+node:get_optimization_matrix_at_site
    def get_optimization_matrix(self):
        return compute_opt_matrix(self.left_boundary,self.operator_matrix,self.right_boundary).reshape((self.system.auxiliary_dimension**2 * self.system.site_dimension,)*2)

        return tensordot(
                 tensordot(self.left_boundary,self.operator_matrix,(2,0)),
                                                        # sum[c''=a''] L_cc'c'' O_a''b''ss' = Y_cc'b''ss'
                 self.right_boundary,(2,2)    # sum[b''=d''] Y_cc'b''ss' R_dd'd'' = Z_cc'ss'dd'
               ).transpose(3,1,5,2,0,4).reshape((self.system.auxiliary_dimension**2 * self.system.site_dimension,)*2)

    #@-node:get_optimization_matrix_at_site
    #@+node:absorb_site
    def absorb_site(self,direction):
        if direction == RIGHT:
            self.right_boundary = special_left_multiply(self.right_boundary,self.system.site_matrix,self.operator_matrix)
        elif direction == LEFT:
            self.left_boundary = special_right_multiply(self.left_boundary,self.system.site_matrix,self.operator_matrix)
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)

        return

    #@+at
    #     left_site_matrix = unitary.reshape((1,)+unitary.shape)
    # 
    #     inverse_unitary = unitary.transpose().conj()
    #     right_site_matrix = 
    # inverse_unitary.reshape((1,)+inverse_unitary.shape)
    # 
    #     operator_matrix = 
    # identity(self.operator_matrix.shape[0]).reshape(self.operator_matrix.shape[:2]+(1,1))
    # 
    #     self.right_boundary = 
    # special_left_multiply(self.right_boundary,right_site_matrix,operator_matrix)
    #     self.left_boundary  = 
    # special_right_multiply(self.left_boundary,left_site_matrix,operator_matrix)
    #@-at
    #@@c
    #@-node:absorb_unitary
    #@-others
#@-node:class MatrixProductOperatorTerm
#@-node:Terms
#@-node:One Dimension
#@+node:Two Dimensions
#@+node:Special multiplication functions
g = Graph()

#@+at
# Note:
# 
# These dimensions are not the actual dimensions;  they are random prime 
# numbers.
# 
# This is done so that index checking can be performed (i.e., to make sure 
# that I've
# connected all of the indices properly).  The function that is produced at 
# the end
# of the day will not depend on these dimensions at all.
# 
#@-at
#@@c

#@+at
# L = Placeholder("L",(7,7,3,101,111))
# A = Placeholder("A",(2,5,7,13,17))
# O = Placeholder("O",(3,3,3,3,2,2))
# R = Placeholder("R",(17,17,3,103,113))
# U = Placeholder("U",(101,103,3,5,5))
# D = Placeholder("D",(111,113,3,13,13))
#@-at
#@@c

L = Placeholder("L",(7,7,3,2,2))
A = Placeholder("A",(2,5,7,13,17))
O = Placeholder("O",(3,3,3,3,2,2))
R = Placeholder("R",(17,17,3,2,2))
U = Placeholder("U",(2,3,2,5,5))
D = Placeholder("D",(3,2,2,13,13))

L_ = g.add_node(L)        #0
A_ = g.add_node(A)        #1
O_ = g.add_node(O)        #2
a_ = g.add_node(A.conj()) #3
R_ = g.add_node(R)        #4
U_ = g.add_node(U)        #5
D_ = g.add_node(D)        #6

# L tensor
g.connect(0,0,1,2) #-->A
g.connect(0,2,2,1) #-->O
g.connect(0,1,3,2) #-->A*
g.connect(0,3,5,0) #-->U
g.connect(0,4,6,1) #-->D

# R tensor
g.connect(4,0,1,4) #-->A
g.connect(4,2,2,3) #-->O
g.connect(4,1,3,4) #-->A*
g.connect(4,3,5,2) #-->U
g.connect(4,4,6,2) #-->D

# U tensor
g.connect(5,1,2,0) #-->O
g.connect(5,3,1,1) #-->A
g.connect(5,4,3,1) #-->A*

# D tensor
g.connect(6,0,2,2) #-->O
g.connect(6,3,1,3) #-->A
g.connect(6,4,3,3) #-->A*

# O tensor
g.connect(2,4,1,0)
g.connect(2,5,3,0)

special_2d_right_multiply = compile_graph(g,[0,1,2,3,5,6],["U","L","D","O","A",],function_name="special_2d_right_multiply")
special_2d_left_multiply = compile_graph(g,[1,2,3,4,5,6],["U","D","R","O","A",],function_name="special_2d_left_multiply")
special_2d_absorb_up = compile_graph(g,[U_,O_,A_,a_],["U","O","A",],node_ordering=[L_,R_,D_],function_name="special_2d_absorb_up")
#compute_transfer_matrix = compile_graph(g,[1,2,3],["A","O"],node_ordering=[0,4],function_name="compute_transfer_matrix")
compute_2d_opt_matrix = compile_graph(g,[0,2,4,5,6],["U","L","D","R","O"],node_ordering=[3,1],function_name="compute_2d_optimization_matrix")

special_2d_matvec = compile_graph(g,[0,1,2,4,5,6],["U","L","D","R","O","A"],function_name="special_2d_matvec")
special_2d_rmatvec = compile_graph(g,[0,2,3,4,5,6],["U","L","D","R","O","A"],function_name="special_2d_rmatvec")

s = Subgraph(g)
for i in xrange(7):
    s.add_node(i)
special_2d_expectation_value = s.merge_all().matrices[0].compile_with_name("special_2d_expectation_value","U","L","D","R","O","A")

special_multiplication_graph = g
del s,g,i


del L,A,O,R,U,D
#@nonl
#@-node:Special multiplication functions
#@+node:class TwoDimensionalSystem
class TwoDimensionalSystem(System):
    #@    @+others
    #@+node:__init__
    def __init__(self,site_dimension,horizontal_auxiliary_dimension,vertical_auxiliary_dimension,initial_up_site_boundary,initial_left_site_boundary,initial_down_site_boundary,initial_right_site_boundary,create_normalization_term=True):
        System.__init__(self)
        self.site_dimension = site_dimension
        self.horizontal_auxiliary_dimension = horizontal_auxiliary_dimension
        self.vertical_auxiliary_dimension = vertical_auxiliary_dimension
        self.initial_up_site_boundary = initial_up_site_boundary
        self.initial_left_site_boundary = initial_left_site_boundary
        self.initial_down_site_boundary = initial_down_site_boundary
        self.initial_right_site_boundary = initial_right_site_boundary

        self.update_quantities_derivative_from_auxiliary_dimension()

        self.site_matrix = 1-2*rand(*self.shape_of_site_matrix)+1j-2j*rand(*self.shape_of_site_matrix)
        self.unnormalized_site_matrix = None

        self.number_of_sites_per_column = 1

        if create_normalization_term:
            self.normalization_term = TwoDimensionalNormalizerTerm(self)
    #@-node:__init__
    #@+node:absorb_site
    def absorb_site(self,direction,number_of_times_to_absorb=1):

        for i in xrange(number_of_times_to_absorb):
            for term in self.terms + [self.normalization_term]:
                term.absorb_site(direction)

        self.total_number_of_sites += self.number_of_sites_per_column


    #@-node:absorb_site
    #@+node:normalize_and_absorb_site
    def normalize_and_absorb_site(self,direction,number_of_times_to_absorb=1):
        self.normalize_site(direction)
        self.absorb_site(direction)
    #@-node:normalize_and_absorb_site
    #@+node:normalize_site
    def normalize_site(self,direction):
        if direction == LEFT:
            self.site_matrix = normalize(self.site_matrix,4)
        elif direction == RIGHT:
            self.site_matrix = normalize(self.site_matrix,2)
        elif direction == UP:
            self.site_matrix = normalize(self.site_matrix,3)
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))
    #@nonl
    #@-node:normalize_site
    #@+node:increase_horizontal_auxiliary_dimension_by
    def increase_horizontal_auxiliary_dimension_by(self,increment):
        u, s, v = svd(2*rand(self.horizontal_auxiliary_dimension,self.horizontal_auxiliary_dimension+increment)-1+2j*rand(self.horizontal_auxiliary_dimension,self.horizontal_auxiliary_dimension+increment)-1j)
        s = diag(ones(v.shape[0]))[:u.shape[0]]
        unitary = reduce(dot,[u,s,v])

        self.horizontal_auxiliary_dimension += increment
        self.update_quantities_derivative_from_auxiliary_dimension()

        if self.site_matrix is not None:
            self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary.conj(),2)
            self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary,4)
            #self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary.conj(),1)
            #self.site_matrix = multiply_tensor_by_matrix_at_index(self.site_matrix,unitary,3)

        if self.unnormalized_site_matrix is not None:
            self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary.conj(),2)
            self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary,4)
            #self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary.conj(),1)
            #self.unnormalized_site_matrix = multiply_tensor_by_matrix_at_index(self.unnormalized_site_matrix,unitary,3)

        for term in [self.normalization_term] + self.terms:
            term.absorb_unitary(unitary)

    #@-node:increase_horizontal_auxiliary_dimension_by
    #@+node:update_quantities_derivative_from_auxiliary_dimension
    def update_quantities_derivative_from_auxiliary_dimension(self):
        self.site_matrix_flattened_dimension = self.site_dimension*self.vertical_auxiliary_dimension**2*self.horizontal_auxiliary_dimension**2
        self.shape_of_site_matrix = (self.site_dimension,)+(self.vertical_auxiliary_dimension,self.horizontal_auxiliary_dimension)*2
    #@-node:update_quantities_derivative_from_auxiliary_dimension
    #@+node:compute_energy_per_term
    def compute_energy_per_term(self):
        boundary = self.normalization_term.get_infinite_limit_boundary()

        return sum([term.get_energy_per_term(*boundary) for term in self.terms])
    #@-node:compute_energy_per_term
    #@+node:absorb_row
    def absorb_row(self,direction):
        #self.normalize_site(UP)

        self.horizontal_auxiliary_dimension = 1
        self.update_quantities_derivative_from_auxiliary_dimension()

        params = self.normalization_term.absorb_row(direction)
        for term in self.terms:
            term.absorb_row(direction,*params)

        self.number_of_sites_per_column += 1
        self.total_number_of_sites = self.number_of_sites_per_column

        self.site_matrix = 1-2*rand(*self.shape_of_site_matrix)+1j-2j*rand(*self.shape_of_site_matrix)
        self.unnormalized_site_matrix = None
    #@-node:absorb_row
    #@+node:replace_boundaries_with_infinite_limit
    def replace_boundaries_with_infinite_limit(self):
        U, L, D, R = self.normalization_term.get_infinite_limit_boundary()
        for term in self.terms + [self.normalization_term]:
            term.replace_boundaries_with(L,R)
        self.total_number_of_sites = 1

    #@-node:replace_boundaries_with_infinite_limit
    #@-others
#@-node:class TwoDimensionalSystem
#@+node:class ProtoPEPSOperatorTerm
class ProtoPEPSOperatorTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,operator_matrix,left_operator_boundary,right_operator_boundary,up_boundary,down_boundary,left_up_boundary,left_down_boundary,right_up_boundary,right_down_boundary):
        self.system = system
        self.operator_matrix = operator_matrix

        self.left_boundary = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            left_operator_boundary,
            left_up_boundary,
            left_down_boundary,
        ])

        self.right_boundary = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            right_operator_boundary,
            right_up_boundary,
            right_down_boundary,
        ])

        self.up_boundary = reduce(outer_product,[
            up_boundary,
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        self.down_boundary = reduce(outer_product,[
            down_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
    #@-node:__init__
    #@+node:get_matvec
    def get_matvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix
        return lambda A: special_2d_matvec(U,L,D,R,O,A)
    #@-node:get_matvec
    #@+node:get_imatvec
    def get_imatvec(self):

        class Dummy:
            pass

        d = Dummy()

        shape_of_site_matrix = self.system.shape_of_site_matrix

        matvec = self.get_matvec()
        d.matvec = lambda A: matvec(A.reshape(shape_of_site_matrix)).ravel()

        #rmatvec = self.get_ratvec()
        #d.rmatvec = lambda A: rmatvec(A.reshape(shape_of_site_matrix)).ravel()

        def imatvec(vector):
            x, info = bicgstab(d,vector.ravel(),tol=10e-10)
            if info != 0:
                raise RuntimeError("info=%i" % i)
            return x.reshape(shape_of_site_matrix)

        return imatvec
    #@-node:get_imatvec
    #@+node:get_rmatvec
    def get_rmatvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix
        return lambda A: special_2d_rmatvec(U,L,D,R,O,A) 

    #@-node:get_rmatvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        return special_2d_expectation_value(
            self.up_boundary,
            self.left_boundary,
            self.down_boundary,
            self.right_boundary,
            self.operator_matrix,
            self.system.site_matrix
        )

    #@-node:get_expectation_value
    #@+node:get_optimization_matrix_at_site
    #@+at
    # def get_optimization_matrix(self):
    #     return 
    # compute_2d_opt_matrix(self.left_boundary,self.operator_matrix,self.right_boundary).reshape((self.system.auxiliary_dimension**2 
    # * self.system.site_dimension,)*2)
    # 
    #     return tensordot(
    #              tensordot(self.left_boundary,self.operator_matrix,(2,0)),
    #                                                     # sum[c''=a''] 
    # L_cc'c'' O_a''b''ss' = Y_cc'b''ss'
    #              self.right_boundary,(2,2)    # sum[b''=d''] Y_cc'b''ss' 
    # R_dd'd'' = Z_cc'ss'dd'
    # ).transpose(3,1,5,2,0,4).reshape((self.system.auxiliary_dimension**2 * 
    # self.system.site_dimension,)*2)
    # 
    #@-at
    #@@c
    #@-node:get_optimization_matrix_at_site
    #@+node:absorb_site
    def absorb_site(self,direction):
        if direction == RIGHT:
            self.right_boundary = special_2d_left_multiply(
                self.up_boundary,
                self.down_boundary,
                self.right_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        elif direction == LEFT:
            self.left_boundary = special_2d_right_multiply(
                self.up_boundary,
                self.left_boundary,
                self.down_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)

    #@+at
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,2)
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),3)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),2)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,3)
    #@-at
    #@@c
        return
    #@-node:absorb_unitary
    #@-others
#@-node:class ProtoPEPSOperatorTerm
#@+node:class PEPSOperatorTerm
class PEPSOperatorTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,operator_matrix,up_operator_boundary,left_operator_boundary,down_operator_boundary,right_operator_boundary):
        self.system = system
        self.operator_matrix = operator_matrix

        self.left_boundary = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            left_operator_boundary,
        ])

        self.right_boundary = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            right_operator_boundary,
        ])

        self.up_boundary = reduce(outer_product,[
            up_operator_boundary,
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        self.down_boundary = reduce(outer_product,[
            down_operator_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
    #@-node:__init__
    #@+node:get_matvec
    def get_matvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix
        return lambda A: special_2d_matvec(U,L,D,R,O,A)
    #@-node:get_matvec
    #@+node:get_imatvec
    def get_imatvec(self):

        class Dummy:
            pass

        d = Dummy()

        shape_of_site_matrix = self.system.shape_of_site_matrix

        matvec = self.get_matvec()
        d.matvec = lambda A: matvec(A.reshape(shape_of_site_matrix)).ravel()

        #rmatvec = self.get_ratvec()
        #d.rmatvec = lambda A: rmatvec(A.reshape(shape_of_site_matrix)).ravel()

        def imatvec(vector):
            x, info = bicgstab(d,vector.ravel(),tol=10e-10)
            if info != 0:
                raise RuntimeError("info=%i" % i)
            return x.reshape(shape_of_site_matrix)

        return imatvec
    #@-node:get_imatvec
    #@+node:get_rmatvec
    def get_rmatvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix
        return lambda A: special_2d_rmatvec(U,L,D,R,O,A) 

    #@-node:get_rmatvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        return special_2d_expectation_value(
            self.up_boundary,
            self.left_boundary,
            self.down_boundary,
            self.right_boundary,
            self.operator_matrix,
            self.system.site_matrix
        )

    #@-node:get_expectation_value
    #@+node:get_optimization_matrix_at_site
    #@+at
    # def get_optimization_matrix(self):
    #     return 
    # compute_2d_opt_matrix(self.left_boundary,self.operator_matrix,self.right_boundary).reshape((self.system.auxiliary_dimension**2 
    # * self.system.site_dimension,)*2)
    # 
    #     return tensordot(
    #              tensordot(self.left_boundary,self.operator_matrix,(2,0)),
    #                                                     # sum[c''=a''] 
    # L_cc'c'' O_a''b''ss' = Y_cc'b''ss'
    #              self.right_boundary,(2,2)    # sum[b''=d''] Y_cc'b''ss' 
    # R_dd'd'' = Z_cc'ss'dd'
    # ).transpose(3,1,5,2,0,4).reshape((self.system.auxiliary_dimension**2 * 
    # self.system.site_dimension,)*2)
    # 
    #@-at
    #@@c
    #@-node:get_optimization_matrix_at_site
    #@+node:absorb_site
    def absorb_site(self,direction):
        if direction == RIGHT:
            self.right_boundary = special_2d_left_multiply(
                self.up_boundary,
                self.down_boundary,
                self.right_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        elif direction == LEFT:
            self.left_boundary = special_2d_right_multiply(
                self.up_boundary,
                self.left_boundary,
                self.down_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)

    #@+at
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,2)
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),3)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),2)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,3)
    #@-at
    #@@c
        return
    #@-node:absorb_unitary
    #@+node:absorb_row
    def absorb_row(self,direction,new_left_site_boundary,new_right_site_boundary):
        assert direction == UP

        U = self.up_boundary
        O = self.operator_matrix
        S = self.system.site_matrix
        s = self.system.site_matrix.conj()

        g = Graph()

        g.add_node(U)
        g.add_node(O)
        g.add_node(S)
        g.add_node(s)


    #@+at
    # def 
    # absorb_row(self,direction,new_left_site_boundary,new_right_site_boundary):
    #     if direction == UP:
    #         newU = 
    # special_2d_absorb_up(self.up_boundary,self.operator_matrix,self.system.site_matrix
    #             ).reshape(
    # self.up_boundary.shape[0]*self.system.site_matrix.shape[2],
    # self.up_boundary.shape[0]*self.system.site_matrix.shape[2],
    #                 self.down_boundary.shape[0],
    #                 self.system.site_matrix.shape[3],
    #                 self.system.site_matrix.shape[3],
    #             ).transpose(
    #                 0,
    #                 2,
    #                 1,
    #                 3,
    #                 4,
    #             )
    # 
    #         newL = multiply.outer(
    #             new_left_site_boundary,      # 0,1,2,3
    #             self.left_operator_boundary, # 4,5,6
    #         ).transpose(
    #             0,
    #             1,
    #             4,
    #             2,5,
    #             3,6,
    #         ).reshape(
    #             new_left_site_boundary.shape[0],
    #             new_left_site_boundary.shape[1],
    #             self.left_operator_boundary.shape[0],
    # new_left_site_boundary.shape[2]*self.left_operator_boundary.shape[1],
    # new_left_site_boundary.shape[3]*self.left_operator_boundary.shape[2],
    #         )
    # 
    #         newR = multiply.outer(
    #             new_right_site_boundary,      # 0,1,2,3
    #             self.right_operator_boundary, # 4,5,6
    #         ).transpose(
    #             0,
    #             1,
    #             4,
    #             2,5,
    #             3,6,
    #         ).reshape(
    #             new_right_site_boundary.shape[0],
    #             new_right_site_boundary.shape[1],
    #             self.right_operator_boundary.shape[0],
    # new_right_site_boundary.shape[2]*self.right_operator_boundary.shape[1],
    # new_right_site_boundary.shape[3]*self.right_operator_boundary.shape[2],
    #         )
    # 
    # 
    #     else:
    #         raise RuntimeError("only absorbing up supported at this time")
    #@-at
    #@@c

    #@-node:absorb_row
    #@-others
#@-node:class PEPSOperatorTerm
#@+node:class MagneticFieldTerm
class MagneticFieldTerm(PEPSOperatorTerm):
    #@    @+others
    #@+node:__init__
    def __init__(self,system):

        self.system = system

        self.left_boundary = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            ones((1,)),
            array([1,0]),
        ])

        self.right_boundary = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            ones((1,)),
            array([0,1]),
        ])

        self.up_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            array([1,0]),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        down_operator_boundary = zeros((2,2,2))
        down_operator_boundary[0,0,0] = 1
        down_operator_boundary[0,1,1] = 1
        down_operator_boundary[1,1,0] = 1

        self.down_boundary = reduce(outer_product,[
            down_operator_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])

        I = identity(2)
        Z = array([[1,0],[0,-1]])

        self.operator_matrix = zeros((2,2,2,2))
        self.operator_matrix[0,0] = I
        self.operator_matrix[0,1] = -Z
        self.operator_matrix[1,1] = I

        self.update_multiplication_functions()


    #@-node:__init__
    #@+node:get_optimization_matrix
    def get_optimization_matrix(self):

        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix

        return self.compute_optimization_matrix(U,L,D,R,O).reshape((prod(self.system.shape_of_site_matrix),)*2)
    #@-node:get_optimization_matrix
    #@+node:get_matvec
    def get_matvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        O = self.operator_matrix
        matvec = self.matvec
        return lambda A: matvec(U,L,D,R,O,A)
    #@-node:get_matvec
    #@+node:get_matvec
    def get_matvec(self):
        O = self.get_optimization_matrix()
        return lambda A: dot(O,A.ravel())
    #@-node:get_matvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        return self.expectation_value(
            self.up_boundary,
            self.left_boundary,
            self.down_boundary,
            self.right_boundary,
            self.operator_matrix,
            self.system.site_matrix
        )

    #@-node:get_expectation_value
    #@+node:absorb_site
    def absorb_site(self,direction):
        if direction == RIGHT:
            self.right_boundary = self.left_multiply(
                self.up_boundary,
                self.down_boundary,
                self.right_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        elif direction == LEFT:
            self.left_boundary = self.right_multiply(
                self.up_boundary,
                self.left_boundary,
                self.down_boundary,
                self.operator_matrix,
                self.system.site_matrix,
            )
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)

    #@+at
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,2)
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),3)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),2)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,3)
    #@-at
    #@@c

        self.update_multiplication_functions()

    #@-node:absorb_unitary
    #@+node:absorb_row
    def absorb_row(self,direction,normalizer_left_boundary,normalizer_right_boundary,X):
        if direction == UP:

            self.up_boundary = self.absorb_up(self.up_boundary,self.operator_matrix,self.system.site_matrix
            ).reshape(
                self.up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
                self.up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
                self.operator_matrix.shape[1],
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
            )

            self.up_boundary = multiply_tensor_by_matrix_at_index(self.up_boundary,X.transpose(),0)
            self.up_boundary = multiply_tensor_by_matrix_at_index(self.up_boundary,X.conj().transpose(),1)

            self.replace_boundaries_with(normalizer_left_boundary,normalizer_right_boundary)

            self.update_multiplication_functions()

        else:
            assert direction == DOWN

            D = self.system.site_matrix.shape[2]**2*self.down_boundary.shape[0]
            #D2 = self.system.site_matrix.shape[4]**2*self.down_boundary.shape[1]

            self.down_boundary = self.absorb_down(self.down_boundary,self.operator_matrix,self.system.site_matrix
            ).reshape(
                D/2,
                2,
                D/2,
                2,
                self.operator_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            self.down_boundary = multiply_tensor_by_matrix_at_index(self.down_boundary,X.transpose(),0)
            self.down_boundary = multiply_tensor_by_matrix_at_index(self.down_boundary,X.conj().transpose(),2)

            self.down_boundary = self.down_boundary.reshape(
                self.down_boundary.shape[0]*self.down_boundary.shape[1],
                self.down_boundary.shape[2]*self.down_boundary.shape[3],
                self.operator_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            self.replace_boundaries_with(normalizer_left_boundary,normalizer_right_boundary)

            self.update_multiplication_functions()


    #@-node:absorb_row
    #@+node:update_multiplication_functions
    def update_multiplication_functions(self):
        #@    @+others
        #@+node:Special magnetic field multiplication functions
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        #@+at
        # L = Placeholder("L",(7,7,3,101,111))
        # A = Placeholder("A",(2,5,7,13,17))
        # O = Placeholder("O",(3,3,3,3,2,2))
        # R = Placeholder("R",(17,17,3,103,113))
        # U = Placeholder("U",(101,103,3,5,5))
        # D = Placeholder("D",(111,113,3,13,13))
        #@-at
        #@@c

        #@+at
        # L = Placeholder("L",(7,7,2,2))
        # A = Placeholder("A",(2,5,7,13,17))
        # O = Placeholder("O",(3,3,2,2))
        # R = Placeholder("R",(17,17,2,2))
        # U = Placeholder("U",(2,2,3,5,5))
        # D = Placeholder("D",(2,2,3,13,13))
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(self.left_boundary.shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(self.right_boundary.shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        O = Placeholder("O",ensure_dimensions_greater_than_one(self.operator_matrix.shape))

        L_ = g.add_node(L)        #0
        A_ = g.add_node(A)        #1
        O_ = g.add_node(O)        #2
        a_ = g.add_node(A.conj()) #3
        R_ = g.add_node(R)        #4
        U_ = g.add_node(U)        #5
        D_ = g.add_node(D)        #6

        # L tensor
        g.connect(L_,0,A_,2) #-->A
        g.connect(L_,1,a_,2) #-->A*
        g.connect(L_,2,U_,0) #-->U
        g.connect(L_,3,D_,0) #-->D

        # R tensor
        g.connect(R_,0,A_,4) #-->A
        g.connect(R_,1,a_,4) #-->A*
        g.connect(R_,2,U_,1) #-->U
        g.connect(R_,3,D_,1) #-->D

        # U tensor
        g.connect(U_,2,O_,0) #-->O
        g.connect(U_,3,A_,1) #-->A
        g.connect(U_,4,a_,1) #-->A*

        # D tensor
        g.connect(D_,2,O_,1) #-->O
        g.connect(D_,3,A_,3) #-->A
        g.connect(D_,4,a_,3) #-->A*

        # O tensor
        g.connect(O_,2,A_,0)
        g.connect(O_,3,a_,0)

        node_ids = {}
        for name in ["U","L","D","R","A","O"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("right_multiply", ["U","L","D","O","A"], [a_], None),
            ("left_multiply", ["U","D","R","O","A"], [a_], None),
            ("matvec",["U","L","D","R","O","A"], [], None),
            ("absorb_up",["U","O","A"], [a_], [L_,R_,D_]),
            ("absorb_down",["D","O","A"], [a_], [L_,R_,U_]),
            ("compute_optimization_matrix", ["U","L","D","R","O"], [], [a_,A_] ),
            ]:
                setattr(self,name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )


        s = Subgraph(g)
        for i in xrange(len(g.nodes)):
            s.add_node(i)
        self.expectation_value = s.merge_all().matrices[0].compile_with_name("expectation_value","U","L","D","R","O","A")

        self.multiplication_graph = g
        del s,g,i


        del L,A,O,R,U,D,L_,A_,a_,O_,R_,U_,D_

        #@-node:Special magnetic field multiplication functions
        #@-others
    #@-node:update_multiplication_functions
    #@+node:replace_boundaries_with
    def replace_boundaries_with(self,normalizer_left_boundary,normalizer_right_boundary):

        horizontal_boundary_shape = copy(normalizer_left_boundary.shape)
        horizontal_boundary_shape[-1] *= 2

        self.left_boundary = multiply.outer(normalizer_left_boundary,array([1,0])).reshape(horizontal_boundary_shape)
        self.right_boundary = multiply.outer(normalizer_right_boundary,array([0,1])).reshape(horizontal_boundary_shape)
    #@-node:replace_boundaries_with
    #@+node:get_energy_per_term
    def get_energy_per_term(self,up_boundary,left_boundary,down_boundary,right_boundary):
        g = Graph()

        U_ = g.add_node(up_boundary)
        L_ = g.add_node(left_boundary)
        D_ = g.add_node(down_boundary)
        R_ = g.add_node(right_boundary)
        O_ = g.add_node(array([[-1,0],[0,1]]))
        S_ = g.add_node(self.system.site_matrix)
        s_ = g.add_node(self.system.site_matrix.conj())

        g.connect(L_,0,S_,2)
        g.connect(L_,1,s_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        g.connect(R_,0,S_,4)
        g.connect(R_,1,s_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        g.connect(U_,2,S_,1)
        g.connect(U_,3,s_,1)

        g.connect(D_,2,S_,3)
        g.connect(D_,3,s_,3)

        g.connect(O_,0,s_,0)
        g.connect(O_,1,S_,0)

        s = Subgraph(g)
        for i in g.nodes.keys():
            s.add_node(i)

        expectation = abs(s.merge_all().matrices[0])

        s = Subgraph(g)
        for i in g.nodes.keys():
            if i == O_:
                s.add_node(O_,identity(2))
            else:
                s.add_node(i)

        normalization = abs(s.merge_all().matrices[0])

        return expectation/normalization
    #@-node:get_energy_per_term
    #@-others
#@-node:class MagneticFieldTerm
#@+node:class TwoDimensionalNormalizerTerm
class TwoDimensionalNormalizerTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system):
        self.system = system

        self.left_boundary = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            ones((1,)),
            ones((1,)),
        ])

        self.right_boundary = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            ones((1,)),
            ones((1,)),
        ])

        self.up_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        self.down_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])

        self.update_multiplication_functions()

    #@-node:__init__
    #@+node:get_optimization_matrix
    def get_optimization_matrix(self):

        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary

        return self.compute_optimization_matrix(U,L,D,R).reshape((prod(self.system.shape_of_site_matrix[1:]),)*2)
    #@-node:get_optimization_matrix
    #@+node:get_matvec
    def get_matvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary

        return lambda A: self.matvec(U,L,D,R,A)

        #A = Placeholder("A",shape=self.system.shape_of_site_matrix)
        #return self.matvec(U,L,D,R,A).compile("A")

    #@-node:get_matvec
    #@+node:get_matvec
    def get_matvec(self):
        O = self.get_optimization_matrix()
        site_matrix_shape = (self.system.shape_of_site_matrix[0],prod(self.system.shape_of_site_matrix[1:]))
        return lambda A: dot(A.reshape(site_matrix_shape),O.transpose())
    #@-node:get_matvec
    #@+node:get_imatvec
    def get_imatvec(self):

        class Dummy:
            pass

        d = Dummy()

        shape_of_site_matrix = self.system.shape_of_site_matrix

        N = self.get_optimization_matrix()

        def imatvec(vector):
            vector = vector.reshape(shape_of_site_matrix)
            x0, info = bicgstab(N,vector[0].ravel(),tol=1e-8) #,maxiter=10) #,tol=1e-10)
            x1, info = bicgstab(N,vector[1].ravel(),tol=1e-8) #,maxiter=10) #,tol=1e-10)
            #if info != 0:
            #    x, info = bicgstab(d,vector.ravel(),tol=1e-6)
            #    if info != 0:
            #        raise RuntimeError("info=%i" % info)
            return array([x0,x1]).reshape(shape_of_site_matrix)

        return imatvec
    #@-node:get_imatvec
    #@+node:get_imatvec
    def get_imatvec(self):

        class Dummy:
            pass

        d = Dummy()

        shape_of_site_matrix = self.system.shape_of_site_matrix

        matvec = self.get_matvec()
        d.matvec = lambda A: matvec(A.reshape(shape_of_site_matrix)).ravel()

        #rmatvec = self.get_ratvec()
        #d.rmatvec = lambda A: rmatvec(A.reshape(shape_of_site_matrix)).ravel()

        def imatvec(vector):
            x, info = bicgstab(d,vector.ravel(),tol=1e-8) #,maxiter=10) #,tol=1e-10)
            #if info != 0:
            #    x, info = bicgstab(d,vector.ravel(),tol=1e-6)
            #    if info != 0:
            #        raise RuntimeError("info=%i" % info)
            return x.reshape(shape_of_site_matrix)

        return imatvec
    #@-node:get_imatvec
    #@+node:get_imatvec
    def get_imatvec(self):

        N = self.get_optimization_matrix()

        Ninv = pinv2(N)

        new_A_shape = (self.system.shape_of_site_matrix[0],Ninv.shape[0])

        return lambda A: dot(A.reshape(new_A_shape),Ninv.transpose())
    #@-node:get_imatvec
    #@+node:get_rmatvec
    def get_rmatvec(self):
        U = self.up_boundary
        L = self.left_boundary
        D = self.down_boundary
        R = self.right_boundary
        return lambda A: self.rmatvec(U,L,D,R,A) 

    #@-node:get_rmatvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        return self.expectation_value(
            self.up_boundary,
            self.left_boundary,
            self.down_boundary,
            self.right_boundary,
            self.system.site_matrix
        )

    #@-node:get_expectation_value
    #@+node:get_infinite_limit_boundary
    def get_infinite_limit_boundary(self):
        U = self.up_boundary
        D = self.down_boundary
        S = self.system.site_matrix

        left_boundary_shape = self.left_boundary.shape
        right_boundary_shape = self.right_boundary.shape

        right_evals, right_evecs = my_eigen(
                lambda R: self.left_multiply(U,D,R.reshape(right_boundary_shape),S).ravel(),
                prod(left_boundary_shape),
                which='LM'
                )

        left_evals, left_evecs = my_eigen(
                lambda L: self.right_multiply(U,L.reshape(left_boundary_shape),D,S).ravel(),
                prod(right_boundary_shape),
                which='LM'
                )

        return U, left_evecs[:,0].reshape(left_boundary_shape), D, right_evecs[:,0].reshape(right_boundary_shape)
    #@-node:get_infinite_limit_boundary
    #@+node:update_multiplication_functions
    def update_multiplication_functions(self):
        #@    @+others
        #@+node:Special normalizer multiplication functions
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        L = Placeholder("L",ensure_dimensions_greater_than_one(self.left_boundary.shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        R = Placeholder("R",ensure_dimensions_greater_than_one(self.right_boundary.shape))
        U = Placeholder("U",ensure_dimensions_greater_than_one(self.up_boundary.shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.down_boundary.shape))

        #@+at
        # L = Placeholder("L",(7,7,2,2))
        # A = Placeholder("A",(2,5,7,13,17))
        # R = Placeholder("R",(17,17,2,2))
        # U = Placeholder("U",(2,2,5,5))
        # D = Placeholder("D",(2,2,13,13))
        #@-at
        #@@c

        L_ = g.add_node(L)        #0
        A_ = g.add_node(A)        #1
        a_ = g.add_node(A.conj()) #2
        R_ = g.add_node(R)        #3
        U_ = g.add_node(U)        #4
        D_ = g.add_node(D)        #5

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,A_,4)
        g.connect(R_,1,a_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,A_,1)
        g.connect(U_,3,a_,1)

        # D tensor
        g.connect(D_,2,A_,3)
        g.connect(D_,3,a_,3)

        # A tensor
        g.connect(A_,0,a_,0)

        node_ids = {}
        for name in ["U","L","D","R","A"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("right_multiply", ["U","L","D","A"], [a_], None),
            ("left_multiply", ["U","D","R","A"], [a_], None),
            ("matvec",["U","L","D","R","A"], [], None),
            ("absorb_up",["U","A"], [a_], [L_,R_,D_]),
            ("absorb_down",["D","A"], [a_], [L_,R_,U_]),
            ("compute_optimization_matrix", ["U","L","D","R"], [], [a_,A_] ),
            ("compute_r_optimization_matrix", ["U","L","D","R"], [], [A_,a_] ),
            ]:
                setattr(self,name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        #@+at
        # self.right_multiply = 
        # compile_graph(g,[0,1,2,4,5],["U","L","D","A",],function_name="right_multiply")
        # self.left_multiply = 
        # compile_graph(g,[1,2,3,4,5],["U","D","R","A",],function_name="left_multiply")
        # 
        # self.matvec = 
        # compile_graph(g,[0,1,3,4,5],["U","L","D","R","A"],function_name="matvec")
        # self.rmatvec = 
        # compile_graph(g,[0,2,3,4,5],["U","L","D","R","A"],function_name="rmatvec")
        # 
        # self.absorb_up = 
        # compile_graph(g,[U_,A_,a_],["U","A",],node_ordering=[L_,R_,D_], 
        # function_name="absorb_up")
        # self.absorb_down = 
        # compile_graph(g,[D_,A_,a_],["D","A",],node_ordering=[L_,R_,U_], 
        # function_name="absorb_down")
        #@-at
        #@@c

        s = Subgraph(g)
        for i in xrange(6):
            s.add_node(i)
        self.expectation_value = s.merge_all().matrices[0].compile_with_name("special_2d_normalizer_expectation_value","U","L","D","R","A")

        self.multiplication_graph = g
        del s, g, i

        del L, A, R, U, D

        #@-node:Special normalizer multiplication functions
        #@+node:Boundary extender function
        #@+at
        # IN = Placeholder("IN",(7,7,111,101))
        # P_UP = Placeholder("P_UP",(111,111,111))
        # P_DOWN = Placeholder("P_DOWN",(101,101,101))
        # 
        # OUT1 = Placeholder("OUT1",(7,))
        # OUT2 = Placeholder("OUT2",(7,))
        # OUT3 = Placeholder("OUT3",(7,))
        # OUT4 = Placeholder("OUT4",(7,))
        # OUT5 = Placeholder("OUT5",(111,))
        # OUT6 = Placeholder("OUT6",(101,))
        # 
        # g = Graph()
        # 
        # IN1_ = g.add_node(IN)
        # IN2_ = g.add_node(IN)
        # P_UP_ = g.add_node(P_UP)
        # P_DOWN_ = g.add_node(P_DOWN)
        # 
        # OUT1_ = g.add_node(OUT1)
        # OUT2_ = g.add_node(OUT2)
        # OUT3_ = g.add_node(OUT3)
        # OUT4_ = g.add_node(OUT4)
        # OUT5_ = g.add_node(OUT5)
        # OUT6_ = g.add_node(OUT6)
        # 
        # g.connect(IN1_,0,OUT1_,0)
        # g.connect(IN1_,1,OUT2_,0)
        # g.connect(IN2_,0,OUT3_,0)
        # g.connect(IN2_,1,OUT4_,0)
        # g.connect(P_UP_,2,OUT5_,0)
        # g.connect(P_DOWN_,2,OUT6_,0)
        # 
        # g.connect(IN1_,2,P_UP_,0)
        # g.connect(IN2_,2,P_UP_,1)
        # 
        # g.connect(IN1_,3,P_DOWN_,0)
        # g.connect(IN2_,3,P_DOWN_,1)
        # 
        # self.extend_indices = 
        # compile_graph(g,[0,1,2,3,],["IN","P_UP","P_DOWN"],node_ordering=[OUT1_,OUT2_,OUT3_,OUT4_,OUT5_,OUT6_,],function_name="special_2d_normalizer_extend_indices")
        # 
        # del 
        # IN,P_UP,P_DOWN,OUT1,OUT2,OUT3,OUT4,OUT5,OUT6,IN1_,IN2_,P_UP_,P_DOWN_,OUT1_,OUT2_,OUT3_,OUT4_,OUT5_,OUT6_,g
        #@-at
        #@@c

        #@-node:Boundary extender function
        #@-others
    #@-node:update_multiplication_functions
    #@+node:absorb_site
    def absorb_site(self,direction):
        if direction == RIGHT:
            self.right_boundary = self.left_multiply(
                self.up_boundary,
                self.down_boundary,
                self.right_boundary,
                self.system.site_matrix,
            )
        elif direction == LEFT:
            self.left_boundary = self.right_multiply(
                self.up_boundary,
                self.left_boundary,
                self.down_boundary,
                self.system.site_matrix,
            )
        else:
            raise RuntimeError("direction must either be LEFT (%i) or RIGHT (%i), instead was %i" % (LEFT,RIGHT,direction))

    #@-node:absorb_site
    #@+node:absorb_row
    def absorb_row(self,direction):
        #@    @+others
        #@+node:reduce_dimension
        def reduce_dimension(matrix,index,new_dimension):
            new_indices = range(matrix.ndim)
            del new_indices[index]
            new_indices.append(index)

            old_shape = list(matrix.shape)
            del old_shape[index]
            new_shape = (prod(old_shape),matrix.shape[index])
            old_shape.append(new_dimension)

            new_matrix = matrix.transpose(new_indices).reshape(new_shape)

            u, s, v = svd(new_matrix,full_matrices=0)

            old_indices = range(matrix.ndim-1)
            old_indices.insert(index,matrix.ndim-1)

            new_u = u[:,:new_dimension]
            new_s = s[:new_dimension]
            new_v = v[:new_dimension,:]

            return (new_s*new_u).reshape(old_shape).transpose(old_indices), new_v

        #@-node:reduce_dimension
        #@-others

        if direction == UP:        
            U, L, D, R = self.get_infinite_limit_boundary()

    #@+at
    #         U = self.up_boundary
    #         L = self.left_boundary
    #         D = self.down_boundary
    #         R = self.right_boundary
    #@-at
    #@@c

            newD = D

            newU = self.absorb_up(U,self.system.site_matrix).reshape(
                self.up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
                self.up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
            )

            newL = L.reshape((1,1,newU.shape[0],newD.shape[0]))

            newR = R.reshape((1,1,newU.shape[0],newD.shape[0]))

            if newU.shape[1] > 16 or True:
                newU, X = reduce_dimension(newU,1,newU.shape[1]-1)
                newU = multiply_tensor_by_matrix_at_index(newU,X.transpose(),0)
                newL = multiply_tensor_by_matrix_at_index(newL,X.transpose().conj(),2)
                newR = multiply_tensor_by_matrix_at_index(newR,X.transpose(),2)
            else:
                X = identity(newU.shape[0])

            self.up_boundary = newU
            self.left_boundary = newL
            self.down_boundary = newD
            self.right_boundary = newR

            self.update_multiplication_functions()

            return newL, newR, X

        elif direction == DOWN:        
            U, L, D, R = self.get_infinite_limit_boundary()

    #@+at
    #         U = self.up_boundary
    #         L = self.left_boundary
    #         D = self.down_boundary
    #         R = self.right_boundary
    #@-at
    #@@c

            newU = U

            newD = self.absorb_down(D,self.system.site_matrix).reshape(
                self.down_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
                self.down_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            newL = L.reshape((1,1,newU.shape[0],newD.shape[0]))

            newR = R.reshape((1,1,newU.shape[0],newD.shape[0]))

            if newD.shape[1] > 16 or True:
                newD, X = reduce_dimension(newD,1,newD.shape[1]-1)
                newD = multiply_tensor_by_matrix_at_index(newD,X.transpose(),0)
                newL = multiply_tensor_by_matrix_at_index(newL,X.transpose().conj(),3)
                newR = multiply_tensor_by_matrix_at_index(newR,X.transpose(),3)
            else:
                X = identity(newD.shape[0])

            self.up_boundary = newU
            self.left_boundary = newL
            self.down_boundary = newD
            self.right_boundary = newR

            self.update_multiplication_functions()

            return newL, newR, X

        else:
            raise RuntimeError("only up/down direction supported")
    #@-node:absorb_row
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
        self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
        self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)

    #@+at
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,2)
    #     self.up_boundary = 
    # multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),3)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),2)
    #     self.down_boundary = 
    # multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,3)
    #@-at
    #@@c

        self.update_multiplication_functions()

    #@-node:absorb_unitary
    #@+node:replace_boundaries_with
    def replace_boundaries_with(self,left_boundary,right_boundary):
        self.left_boundary = left_boundary
        self.right_boundary = right_boundary
    #@-node:replace_boundaries_with
    #@-others
#@-node:class TwoDimensionalNormalizerTerm
#@+node:class SpinCouplingTerm
class SpinCouplingTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,lam):
        self.system = system
        self.lam = lam
        self.number_of_sites_absorbed_left = 0
        self.number_of_sites_absorbed_right = 0
        self.left_site = None
        self.right_site = None

        I = identity(2)
        X = array([[0,1],[1,0]])

        #@    << One site boundary >>
        #@+node:<< One site boundary >>
        self.one_site_up_boundary = reduce(outer_product,[
            ones((1,1)),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        self.one_site_down_boundary = reduce(outer_product,[
            identity(2),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
        #@-node:<< One site boundary >>
        #@nl

        multi_site_down_boundary = zeros((2,2,2))
        multi_site_down_boundary[0,0,0] = 1
        multi_site_down_boundary[0,1,1] = 1
        multi_site_down_boundary[1,1,0] = 1

        #@    << Two site boundary >>
        #@+node:<< Two site boundary >>
        self.up_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            array([1,0]),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        two_site_operator_matrix_A = zeros((2,2,2,)+(2,2))
        two_site_operator_matrix_A[0,0,0] = I
        two_site_operator_matrix_A[0,1,1] = -lam*X
        two_site_operator_matrix_A[1,1,0] = I

        two_site_operator_matrix_B = zeros((2,)+(2,2))
        two_site_operator_matrix_B[0] = I
        two_site_operator_matrix_B[1] = X

        self.lamXX = multiply.outer(-lam*X,X)

        self.operator_matrix_1 = two_site_operator_matrix_A
        self.operator_matrix_2 = two_site_operator_matrix_B

        self.down_boundary = reduce(outer_product,[
            multi_site_down_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
        #@-node:<< Two site boundary >>
        #@nl

        self.left_boundary = [None,None]
        self.left_boundary[0] = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            ones((1,)),
            array([1,0]),
        ])
        self.left_boundary[1] = self.left_boundary[0].copy()

        self.right_boundary = [None,None]
        self.right_boundary[0] = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            ones((1,)),
            array([0,1]),
        ])
        self.right_boundary[1] = self.right_boundary[0].copy()

        self.update_multiplication_functions()


    #@-node:__init__
    #@+node:get_optimization_matrix
    def get_optimization_matrix(self):
        optimization_matrix_shape = self.system.shape_of_site_matrix*2
        optimization_matrix = zeros(optimization_matrix_shape,complex128)
        U = self.up_boundary
        D = self.down_boundary
        O1 = self.operator_matrix_1
        O2 = self.operator_matrix_2

        fns = []

        if self.left_site is not None:
            A = self.left_site
            L_AB = self.left_boundary[(self.number_of_sites_absorbed_left-1)%2]
            R_AB = self.right_boundary[(self.number_of_sites_absorbed_right)%2]
            optimization_matrix += self.compute_optimization_matrix_AB(U,L_AB,D,R_AB,O1,O2,A)

        if self.right_site is not None:
            C = self.right_site
            L_BC = self.left_boundary[(self.number_of_sites_absorbed_left)%2]
            R_BC = self.right_boundary[(self.number_of_sites_absorbed_right-1)%2]
            optimization_matrix += self.compute_optimization_matrix_BC(U,L_BC,D,R_BC,O1,O2,C)

        return optimization_matrix.reshape((prod(self.system.shape_of_site_matrix),)*2)
    #@-node:get_optimization_matrix
    #@+node:get_matvec
    def get_matvec(self):
        U = self.up_boundary
        D = self.down_boundary
        O1 = self.operator_matrix_1
        O2 = self.operator_matrix_2

        fns = []

        if self.left_site is not None:
            A = self.left_site
            L_AB = self.left_boundary[(self.number_of_sites_absorbed_left-1)%2]
            R_AB = self.right_boundary[(self.number_of_sites_absorbed_right)%2]
            fns.append(lambda B1: self.matvec_AB(U,L_AB,D,R_AB,O1,O2,A,B1))

        if self.right_site is not None:
            C = self.right_site
            L_BC = self.left_boundary[(self.number_of_sites_absorbed_left)%2]
            R_BC = self.right_boundary[(self.number_of_sites_absorbed_right-1)%2]
            fns.append(lambda B2: self.matvec_BC(U,L_BC,D,R_BC,O1,O2,B2,C))

        if len(fns) == 0:
            return None
        elif len(fns) == 1:
            return fns[0]
        else:
            assert len(fns) == 2
            return lambda B: fns[0](B) + fns[1](B)
    #@-node:get_matvec
    #@+node:get_matvec
    def get_matvec(self):
        O = self.get_optimization_matrix()
        return lambda A: dot(O,A.ravel())
    #@-node:get_matvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        U = self.up_boundary
        D = self.down_boundary
        O1 = self.operator_matrix_1
        O2 = self.operator_matrix_2

        value = 0

        B = self.system.site_matrix

        if self.left_site is not None:
            A = self.left_site
            L = self.left_boundary[(self.number_of_sites_absorbed_left-1)%2]
            R = self.right_boundary[(self.number_of_sites_absorbed_right)%2]
            value += self.expectation_value(U,L,D,R,O1,O2,A,B)

        if self.right_site is not None:
            C = self.right_site
            L = self.left_boundary[(self.number_of_sites_absorbed_left)%2]
            R = self.right_boundary[(self.number_of_sites_absorbed_right-1)%2]
            value += self.expectation_value(U,L,D,R,O1,O2,B,C)

        return value

    #@-node:get_expectation_value
    #@+node:get_energy_per_term
    def get_energy_per_term(self,up_boundary,left_boundary,down_boundary,right_boundary):
        g = Graph()

        U1_ = g.add_node(up_boundary)
        U2_ = g.add_node(up_boundary)
        L_ = g.add_node(left_boundary)
        D1_ = g.add_node(down_boundary)
        D2_ = g.add_node(down_boundary)
        R_ = g.add_node(right_boundary)
        O_ = g.add_node(self.lamXX)
        S1_ = g.add_node(self.system.site_matrix)
        s1_ = g.add_node(self.system.site_matrix.conj())
        S2_ = g.add_node(self.system.site_matrix)
        s2_ = g.add_node(self.system.site_matrix.conj())

        g.connect(L_,0,S1_,2)
        g.connect(L_,1,s1_,2)
        g.connect(L_,2,U1_,0)
        g.connect(L_,3,D1_,0)

        g.connect(R_,0,S2_,4)
        g.connect(R_,1,s2_,4)
        g.connect(R_,2,U2_,1)
        g.connect(R_,3,D2_,1)

        g.connect(U1_,1,U2_,0)
        g.connect(U1_,2,S1_,1)
        g.connect(U1_,3,s1_,1)
        g.connect(U2_,2,S2_,1)
        g.connect(U2_,3,s2_,1)

        g.connect(D1_,1,D2_,0)
        g.connect(D1_,2,S1_,3)
        g.connect(D1_,3,s1_,3)
        g.connect(D2_,2,S2_,3)
        g.connect(D2_,3,s2_,3)

        g.connect(O_,0,S1_,0)
        g.connect(O_,1,s1_,0)
        g.connect(O_,2,S2_,0)
        g.connect(O_,3,s2_,0)

        g.connect(S1_,4,S2_,2)
        g.connect(s1_,4,s2_,2)

        s = Subgraph(g)
        for i in g.nodes.keys():
            s.add_node(i)

        expectation = abs(s.merge_all().matrices[0])

        s = Subgraph(g)
        for i in g.nodes.keys():
            if i == O_:
                s.add_node(O_,outer_product(*[identity(2)]*2))
            else:
                s.add_node(i)

        normalization = abs(s.merge_all().matrices[0])

        return expectation/normalization
    #@-node:get_energy_per_term
    #@+node:update_multiplication_functions
    def update_multiplication_functions(self,horizontal_boundary_shape=None):
        if horizontal_boundary_shape is None:
            horizontal_boundary_shape = self.left_boundary[0].shape
        #@    @+others
        #@+node:Special 2 body multiplication functions - one site
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.one_site_up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.one_site_down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))

        U_ = g.add_node(U)        #0
        L_ = g.add_node(L)        #1
        D_ = g.add_node(D)        #2
        R_ = g.add_node(R)        #3
        A_ = g.add_node(A)        #4
        a_ = g.add_node(A.conj()) #5

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,A_,4)
        g.connect(R_,1,a_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,A_,1)
        g.connect(U_,3,a_,1)

        # D tensor
        g.connect(D_,2,A_,3)
        g.connect(D_,3,a_,3)

        # A tensor, A* tensor
        g.connect(A_,0,a_,0)

        node_ids = {}
        for name in ["U","L","D","R","A"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("absorb_one_site_into_left_boundary", ["U","L","D","A"], [a_], None),
            ("absorb_one_site_into_right_boundary", ["U","D","R","A"], [a_], None),
            ("absorb_one_site_up", ["U", "A"], [a_], [L_, R_, D_]),
            ("absorb_one_site_down", ["D", "A"], [a_], [L_, R_, U_]),
            ]:
                setattr(self,name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        del g

        del U,L,D,R,A,U_,L_,D_,R_,A_,a_

        #@-node:Special 2 body multiplication functions - one site
        #@+node:Special 2 body multiplication functions - two sites
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        #@+at
        # U = Placeholder("U",(6,3,7,5,5,5,5))
        # L = Placeholder("L",(6,8,5,5))
        # D = Placeholder("D",(3,8,9,5,5,5,5))
        # R = Placeholder("R",(7,9,5,5))
        # O = Placeholder("O",(3,3,2,2,2,2))
        # A = Placeholder("A",(2,5,5,5,5))
        # B = Placeholder("B",(2,5,5,5,5))
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        OA = Placeholder("OA",ensure_dimensions_greater_than_one(self.operator_matrix_1.shape))
        OB = Placeholder("OB",ensure_dimensions_greater_than_one(self.operator_matrix_2.shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        B = Placeholder("B",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))

        U_ = g.add_node(U)
        L_ = g.add_node(L)
        D_ = g.add_node(D)
        R_ = g.add_node(R)
        OA_= g.add_node(OA)
        OB_= g.add_node(OB)
        A_ = g.add_node(A)
        a_ = g.add_node(A.conj())
        B_ = g.add_node(B)
        b_ = g.add_node(B.conj())

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,B_,4)
        g.connect(R_,1,b_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,OA_,0)
        g.connect(U_,3,A_,1)
        g.connect(U_,4,a_,1)
        g.connect(U_,5,B_,1)
        g.connect(U_,6,b_,1)

        # D tensor
        g.connect(D_,2,OA_,1)
        g.connect(D_,3,A_,3)
        g.connect(D_,4,a_,3)
        g.connect(D_,5,B_,3)
        g.connect(D_,6,b_,3)

        # O tensor
        g.connect(OA_,3,A_,0)
        g.connect(OA_,4,a_,0)
        g.connect(OB_,1,B_,0)
        g.connect(OB_,2,b_,0)
        g.connect(OA_,2,OB_,0)

        # A tensor, A* tensor
        g.connect(A_,4,B_,2)
        g.connect(a_,4,b_,2)

        node_ids = {}
        for name in ["U","L","D","R","OA","OB","A","B"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("matvec_AB", ["U","L","D","R","OA","OB","A","B"], [a_], None),
            ("matvec_BC", ["U","L","D","R","OA","OB","A","B"], [b_], None),
            ("compute_optimization_matrix_AB", ["U","L","D","R","OA","OB","A"], [a_], [b_,B_]),
            ("compute_optimization_matrix_BC", ["U","L","D","R","OA","OB","B"], [b_], [a_,A_]),
            ("absorb_two_sites_into_left_boundary", ["U","L","D","OA","OB","A","B"], [a_,b_], None),
            ("absorb_two_sites_into_right_boundary", ["U","D","R","OA","OB","A","B"], [a_,b_], None),
            ("absorb_two_sites_up", ["U", "OA", "OB", "A", "B"], [a_, b_], [L_, R_, D_]),
            ("absorb_two_sites_down", ["D", "OA", "OB", "A", "B"], [a_, b_], [L_, R_, U_]),
            ]:
                setattr(self,name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        s = Subgraph(g)
        for i in xrange(len(g.nodes)):
            s.add_node(i)
        self.expectation_value = s.merge_all().matrices[0].compile_with_name("expectation_value","U","L","D","R","OA","OB","A","B")

        del s, g, i

        del U,L,D,R,A,U_,L_,D_,R_,A_,a_,B_,b_

        #@-node:Special 2 body multiplication functions - two sites
        #@-others
    #@-node:update_multiplication_functions
    #@+node:absorb_row
    def absorb_row(self,direction,normalizer_left_boundary,normalizer_right_boundary,X):
        if direction == UP:

            self.up_boundary = self.absorb_two_sites_up(
                self.up_boundary,
                self.operator_matrix_1,
                self.operator_matrix_2,
                self.system.site_matrix,
                self.system.site_matrix,
            ).reshape(
                self.up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
                self.up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
                self.operator_matrix_1.shape[0],
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
            )

            self.up_boundary = multiply_tensor_by_matrix_at_index(self.up_boundary,X.transpose(),0)
            self.up_boundary = multiply_tensor_by_matrix_at_index(self.up_boundary,X.conj().transpose(),1)

            self.one_site_up_boundary = self.absorb_one_site_up(
                self.one_site_up_boundary,
                self.system.site_matrix
            ).reshape(
                self.one_site_up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
                self.one_site_up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
                self.system.site_matrix.shape[3],
                self.system.site_matrix.shape[3],
            )

            self.one_site_up_boundary = multiply_tensor_by_matrix_at_index(self.one_site_up_boundary,X.transpose(),0)
            self.one_site_up_boundary = multiply_tensor_by_matrix_at_index(self.one_site_up_boundary,X.conj().transpose(),1)

            self.replace_boundaries_with(normalizer_left_boundary,normalizer_right_boundary)

            self.update_multiplication_functions()

        else:

            assert direction == DOWN

            D = self.down_boundary.shape[0]*self.system.site_matrix.shape[2]**2
            #D2 = self.down_boundary.shape[1]*self.system.site_matrix.shape[4]**2

            self.down_boundary = self.absorb_two_sites_down(
                self.down_boundary,
                self.operator_matrix_1,
                self.operator_matrix_2,
                self.system.site_matrix,
                self.system.site_matrix,
            ).reshape(
                D/2,
                2,
                D/2,
                2,
                self.operator_matrix_1.shape[0],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            self.down_boundary = multiply_tensor_by_matrix_at_index(self.down_boundary,X.transpose(),0)
            self.down_boundary = multiply_tensor_by_matrix_at_index(self.down_boundary,X.conj().transpose(),2)

            self.down_boundary = self.down_boundary.reshape(
                self.down_boundary.shape[0]*self.down_boundary.shape[1],
                self.down_boundary.shape[2]*self.down_boundary.shape[3],
                self.operator_matrix_1.shape[0],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            D = self.one_site_down_boundary.shape[0]*self.system.site_matrix.shape[2]**2
            #D2 = self.one_site_down_boundary.shape[1]*self.system.site_matrix.shape[4]**2

            self.one_site_down_boundary = self.absorb_one_site_down(
                self.one_site_down_boundary,
                self.system.site_matrix
            ).reshape(
                D/2,
                2,
                D/2,
                2,
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            self.one_site_down_boundary = multiply_tensor_by_matrix_at_index(self.one_site_down_boundary,X.transpose(),0)
            self.one_site_down_boundary = multiply_tensor_by_matrix_at_index(self.one_site_down_boundary,X.conj().transpose(),2)

            self.one_site_down_boundary = self.one_site_down_boundary.reshape(
                self.one_site_down_boundary.shape[0]*self.one_site_down_boundary.shape[1],
                self.one_site_down_boundary.shape[2]*self.one_site_down_boundary.shape[3],
                self.system.site_matrix.shape[1],
                self.system.site_matrix.shape[1],
            )

            self.replace_boundaries_with(normalizer_left_boundary,normalizer_right_boundary)

            self.update_multiplication_functions()

    #@-node:absorb_row
    #@+node:absorb_site
    def absorb_site(self,direction):
        assert direction == LEFT or direction == RIGHT
        if direction == LEFT:
            self.number_of_sites_absorbed_left += 1       
            if self.left_site is not None:
                self.left_boundary[self.number_of_sites_absorbed_left%2] = \
                    self.absorb_two_sites_into_left_boundary(
                        self.up_boundary,
                        self.left_boundary[self.number_of_sites_absorbed_left%2],
                        self.down_boundary,
                        self.operator_matrix_1,
                        self.operator_matrix_2,
                        self.left_site,
                        self.system.site_matrix
                        )
            else:
                self.left_boundary[1] = \
                    self.absorb_one_site_into_left_boundary(
                        self.one_site_up_boundary,
                        self.left_boundary[1],
                        self.one_site_down_boundary,
                        self.system.site_matrix
                        )     
            self.left_site = self.system.site_matrix
        else:
            self.number_of_sites_absorbed_right += 1
            if self.right_site is not None:
                self.right_boundary[self.number_of_sites_absorbed_right%2] = \
                    self.absorb_two_sites_into_right_boundary(
                        self.up_boundary,
                        self.down_boundary,
                        self.right_boundary[self.number_of_sites_absorbed_right%2],
                        self.operator_matrix_1,
                        self.operator_matrix_2,
                        self.system.site_matrix,
                        self.right_site,
                        )
            else:
                self.right_boundary[1] = \
                    self.absorb_one_site_into_right_boundary(
                        self.one_site_up_boundary,
                        self.one_site_down_boundary,
                        self.right_boundary[1],
                        self.system.site_matrix
                        )    
            self.right_site = self.system.site_matrix
    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):
        if self.left_site is not None:
            self.left_site = multiply_tensor_by_matrix_at_index(self.left_site,unitary,4)
            i = self.number_of_sites_absorbed_left%2
            self.left_boundary[i] = multiply_tensor_by_matrix_at_index(self.left_boundary[i],unitary,0)
            self.left_boundary[i] = multiply_tensor_by_matrix_at_index(self.left_boundary[i],unitary.conj(),1)
        else:
            for i in (0,1):
                self.left_boundary[i] = multiply_tensor_by_matrix_at_index(self.left_boundary[i],unitary,0)
                self.left_boundary[i] = multiply_tensor_by_matrix_at_index(self.left_boundary[i],unitary.conj(),1)

        if self.right_site is not None:
            self.right_site = multiply_tensor_by_matrix_at_index(self.right_site,unitary.conj(),2)
            i = self.number_of_sites_absorbed_right%2
            self.right_boundary[i] = multiply_tensor_by_matrix_at_index(self.right_boundary[i],unitary.conj(),0)
            self.right_boundary[i] = multiply_tensor_by_matrix_at_index(self.right_boundary[i],unitary,1)
        else:
            for i in (0,1):
                self.right_boundary[i] = multiply_tensor_by_matrix_at_index(self.right_boundary[i],unitary,0)
                self.right_boundary[i] = multiply_tensor_by_matrix_at_index(self.right_boundary[i],unitary.conj(),1)

        horizontal_boundary_shape = list(self.left_boundary[0].shape)
        horizontal_boundary_shape[0] = horizontal_boundary_shape[1] = self.system.shape_of_site_matrix[2]
        self.update_multiplication_functions(tuple(horizontal_boundary_shape))
    #@-node:absorb_unitary
    #@+node:replace_boundaries_with
    def replace_boundaries_with(self,normalizer_left_boundary,normalizer_right_boundary):
        self.number_of_left_sites_absorbed = 0
        self.number_of_right_sites_absorbed = 0
        self.left_site = None
        self.right_site = None

        horizontal_boundary_shape = copy(normalizer_left_boundary.shape)
        horizontal_boundary_shape[-1] *= 2

        for i in (0,1):
            self.left_boundary[i] = multiply.outer(normalizer_left_boundary,array([1,0])).reshape(horizontal_boundary_shape)
            self.right_boundary[i] = multiply.outer(normalizer_right_boundary,array([0,1])).reshape(horizontal_boundary_shape)
    #@-node:replace_boundaries_with
    #@-others
#@-node:class SpinCouplingTerm
#@+node:class OldSpinCouplingTerm
class OldSpinCouplingTerm:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,lam,ignore_first_left=False,ignore_first_right=False,ignore_first_two_matvecs=False):
        self.system = system
        self.lam = lam
        self.number_of_active_sites = 1
        self.ignore_first_left = ignore_first_left
        self.ignore_next_left_absorb = ignore_first_left
        self.ignore_first_right = ignore_first_right
        self.ignore_next_right_absorb = ignore_first_right
        self.ignore_first_two_matvecs = ignore_first_two_matvecs
        self.matvecs_to_ignore = 2 if ignore_first_two_matvecs else 0

        I = identity(2)
        X = array([[0,1],[1,0]])

        #@    << One site boundary >>
        #@+node:<< One site boundary >>
        self.one_site_up_boundary = reduce(outer_product,[
            ones((1,1)),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        self.one_site_down_boundary = reduce(outer_product,[
            identity(2),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
        #@-node:<< One site boundary >>
        #@nl

        multi_site_down_boundary = zeros((2,2,2))
        multi_site_down_boundary[0,0,0] = 1
        multi_site_down_boundary[0,1,1] = 1
        multi_site_down_boundary[1,1,0] = 1

        #@    << Two site boundary >>
        #@+node:<< Two site boundary >>
        self.two_site_up_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            array([1,0]),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        #II = reduce(outer_product,[identity(2)]*2)
        #XX = reduce(outer_product,[array([[0,1],[1,0]])]*2)

        two_site_operator_matrix_A = zeros((2,2,2,)+(2,2))
        two_site_operator_matrix_A[0,0,0] = I
        two_site_operator_matrix_A[0,1,1] = -lam*X
        two_site_operator_matrix_A[1,1,0] = I

        two_site_operator_matrix_B = zeros((2,)+(2,2))
        two_site_operator_matrix_B[0] = I
        two_site_operator_matrix_B[1] = X

        self.lamXX = multiply.outer(-lam*X,X)

        self.two_site_operator_A = two_site_operator_matrix_A
        self.two_site_operator_B = two_site_operator_matrix_B

        self.two_site_down_boundary = reduce(outer_product,[
            multi_site_down_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
        #@-node:<< Two site boundary >>
        #@nl
        #@    << Three site boundary >>
        #@+node:<< Three site boundary >>
        self.three_site_up_boundary = reduce(outer_product,[
            ones((1,)),
            ones((1,)),
            array([1,0]),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
            system.initial_up_site_boundary,
            system.initial_up_site_boundary.conj(),
        ])

        #@+at
        # III = reduce(outer_product,[identity(2)]*3)
        # XX = reduce(outer_product,[array([[0,1],[1,0]])]*2)
        # 
        # IXX = outer_product(identity(2),XX)
        # XXI = outer_product(XX,identity(2))
        # 
        # three_site_operator_matrix = zeros((2,2,2,2,2,2,2,2))
        # three_site_operator_matrix[0,0] = III
        # three_site_operator_matrix[0,1] = -lam*(XXI+IXX)
        # three_site_operator_matrix[1,1] = III
        #@-at
        #@@c

        three_site_operator_matrix_A = zeros((2,2,3,)+(2,2))
        three_site_operator_matrix_A[0,0,0] = I
        three_site_operator_matrix_A[0,1,1] = -lam*X
        three_site_operator_matrix_A[0,1,2] = I
        three_site_operator_matrix_A[1,1,0] = I

        three_site_operator_matrix_B = zeros((3,2,)+(2,2))
        three_site_operator_matrix_B[0,0] = I
        three_site_operator_matrix_B[1,0] = X
        three_site_operator_matrix_B[2,1] = -lam*X

        three_site_operator_matrix_C = zeros((2,)+(2,2))
        three_site_operator_matrix_C[0] = I
        three_site_operator_matrix_C[1] = X

        self.three_site_operator_A = three_site_operator_matrix_A
        self.three_site_operator_B = three_site_operator_matrix_B
        self.three_site_operator_C = three_site_operator_matrix_C

        self.three_site_down_boundary = reduce(outer_product,[
            multi_site_down_boundary,
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
            system.initial_down_site_boundary,
            system.initial_down_site_boundary.conj(),
        ])
        #@-node:<< Three site boundary >>
        #@nl

        self.left_boundary = reduce(outer_product,[
            system.initial_left_site_boundary,
            system.initial_left_site_boundary.conj(),
            ones((1,)),
            array([1,0]),
        ])

        self.right_boundary = reduce(outer_product,[
            system.initial_right_site_boundary,
            system.initial_right_site_boundary.conj(),
            ones((1,)),
            array([0,1]),
        ])

        self.update_multiplication_functions()


    #@-node:__init__
    #@+node:get_matvec
    def get_matvec(self):
        if self.matvecs_to_ignore > 0:
            return None
        L = self.left_boundary
        R = self.right_boundary
        if self.number_of_active_sites == 1:
            return None
            U = self.one_site_up_boundary
            D = self.one_site_down_boundary
            return lambda A: self.one_site_matvec(U,L,D,R,A)
        elif self.number_of_active_sites == 2:
            U = self.two_site_up_boundary
            D = self.two_site_down_boundary
            OA = self.two_site_operator_A
            OB = self.two_site_operator_B
            if self.A is None:
                B = self.B
                assert B is not None
                return lambda A: self.two_site_matvec_A(U,L,D,R,OA,OB,A,B)
            else:
                A = self.A
                assert A is not None
                return lambda B: self.two_site_matvec_B(U,L,D,R,OA,OB,A,B)
        elif self.number_of_active_sites == 3:
            return None
            U = self.three_site_up_boundary
            D = self.three_site_down_boundary
            OA = self.three_site_operator_A
            OB = self.three_site_operator_B
            OC = self.three_site_operator_C
            assert self.A is not None
            assert self.B is None
            assert self.C is not None

            A = self.A
            C = self.C

            return lambda B: self.three_site_matvec_B(U,L,D,R,OA,OB,OC,A,B,C)
        else:
            raise RuntimeError("shouldn't ever get here (self.number_of_active_sites=%i" % self.number_of_active_sites)
    #@-node:get_matvec
    #@+node:get_expectation_value
    def get_expectation_value(self):
        if self.matvecs_to_ignore > 0:
            return 0
        if self.number_of_active_sites == 1:
            return 0
            return self.one_site_expectation_value(
                self.one_site_up_boundary,
                self.left_boundary,
                self.one_site_down_boundary,
                self.right_boundary,
                self.system.site_matrix,
                )
        elif self.number_of_active_sites == 2:
            if self.A is None:
                assert self.B is not None
                return self.two_site_expectation_value(
                    self.two_site_up_boundary,
                    self.left_boundary,
                    self.two_site_down_boundary,
                    self.right_boundary,
                    self.two_site_operator_A,
                    self.two_site_operator_B,
                    self.system.site_matrix,
                    self.B,
                    )
            else:
                assert self.A is not None
                return self.two_site_expectation_value(
                    self.two_site_up_boundary,
                    self.left_boundary,
                    self.two_site_down_boundary,
                    self.right_boundary,
                    self.two_site_operator_A,
                    self.two_site_operator_B,
                    self.A,
                    self.system.site_matrix,
                    )
        else:
            return 0
            assert self.number_of_active_sites == 3
            assert self.A is not None
            assert self.B is None
            assert self.C is not None
            return self.three_site_expectation_value(
                self.three_site_up_boundary,
                self.left_boundary,
                self.three_site_down_boundary,
                self.right_boundary,
                self.three_site_operator_A,
                self.three_site_operator_B,
                self.three_site_operator_C,
                self.A,
                self.system.site_matrix,
                self.C,
                )

    #@-node:get_expectation_value
    #@+node:get_energy_per_term
    def get_energy_per_term(self,up_boundary,left_boundary,down_boundary,right_boundary):
        if self.matvecs_to_ignore > 0 or self.number_of_active_sites % 2 == 1:
            return 0

        g = Graph()

        U1_ = g.add_node(up_boundary)
        U2_ = g.add_node(up_boundary)
        L_ = g.add_node(left_boundary)
        D1_ = g.add_node(down_boundary)
        D2_ = g.add_node(down_boundary)
        R_ = g.add_node(right_boundary)
        O_ = g.add_node(self.lamXX/2)
        S1_ = g.add_node(self.system.site_matrix)
        s1_ = g.add_node(self.system.site_matrix.conj())
        S2_ = g.add_node(self.system.site_matrix)
        s2_ = g.add_node(self.system.site_matrix.conj())

        g.connect(L_,0,S1_,2)
        g.connect(L_,1,s1_,2)
        g.connect(L_,2,U1_,0)
        g.connect(L_,3,D1_,0)

        g.connect(R_,0,S2_,4)
        g.connect(R_,1,s2_,4)
        g.connect(R_,2,U2_,1)
        g.connect(R_,3,D2_,1)

        g.connect(U1_,1,U2_,0)
        g.connect(U1_,2,S1_,1)
        g.connect(U1_,3,s1_,1)
        g.connect(U2_,2,S2_,1)
        g.connect(U2_,3,s2_,1)

        g.connect(D1_,1,D2_,0)
        g.connect(D1_,2,S1_,3)
        g.connect(D1_,3,s1_,3)
        g.connect(D2_,2,S2_,3)
        g.connect(D2_,3,s2_,3)

        g.connect(O_,0,S1_,0)
        g.connect(O_,1,s1_,0)
        g.connect(O_,2,S2_,0)
        g.connect(O_,3,s2_,0)

        g.connect(S1_,4,S2_,2)
        g.connect(s1_,4,s2_,2)

        s = Subgraph(g)
        for i in g.nodes.keys():
            s.add_node(i)

        expectation = abs(s.merge_all().matrices[0])

        s = Subgraph(g)
        for i in g.nodes.keys():
            if i == O_:
                s.add_node(O_,outer_product(*[identity(2)]*2))
            else:
                s.add_node(i)

        normalization = abs(s.merge_all().matrices[0])

        return expectation/normalization
    #@-node:get_energy_per_term
    #@+node:update_multiplication_functions
    def update_multiplication_functions(self,horizontal_boundary_shape=None):
        if horizontal_boundary_shape is None:
            horizontal_boundary_shape = self.left_boundary.shape
        #@    @+others
        #@+node:Special 2 body multiplication functions - one site
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.one_site_up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.one_site_down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))

        U_ = g.add_node(U)        #0
        L_ = g.add_node(L)        #1
        D_ = g.add_node(D)        #2
        R_ = g.add_node(R)        #3
        A_ = g.add_node(A)        #4
        a_ = g.add_node(A.conj()) #5

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,A_,4)
        g.connect(R_,1,a_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,A_,1)
        g.connect(U_,3,a_,1)

        # D tensor
        g.connect(D_,2,A_,3)
        g.connect(D_,3,a_,3)

        # A tensor, A* tensor
        g.connect(A_,0,a_,0)

        node_ids = {}
        for name in ["U","L","D","R","A"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("matvec", ["U","L","D","R","A",], [], None),
            ("absorb_into_left_boundary", ["U","L","D","A"], [a_], None),
            ("absorb_into_right_boundary", ["U","D","R","A"], [a_], None),
            ("absorb_up", ["U", "A"], [a_], [L_, R_, D_]),
            ]:
                setattr(self,"one_site_"+name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        s = Subgraph(g)
        for i in xrange(len(g.nodes)):
            s.add_node(i)
        self.one_site_expectation_value = s.merge_all().matrices[0].compile_with_name("expectation_value","U","L","D","R","A")

        del s, g, i

        del U,L,D,R,A,U_,L_,D_,R_,A_,a_

        #@-node:Special 2 body multiplication functions - one site
        #@+node:Special 2 body multiplication functions - two sites
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        #@+at
        # U = Placeholder("U",(6,3,7,5,5,5,5))
        # L = Placeholder("L",(6,8,5,5))
        # D = Placeholder("D",(3,8,9,5,5,5,5))
        # R = Placeholder("R",(7,9,5,5))
        # O = Placeholder("O",(3,3,2,2,2,2))
        # A = Placeholder("A",(2,5,5,5,5))
        # B = Placeholder("B",(2,5,5,5,5))
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.two_site_up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.two_site_down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        OA = Placeholder("OA",ensure_dimensions_greater_than_one(self.two_site_operator_A.shape))
        OB = Placeholder("OB",ensure_dimensions_greater_than_one(self.two_site_operator_B.shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        B = Placeholder("B",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))

        U_ = g.add_node(U)
        L_ = g.add_node(L)
        D_ = g.add_node(D)
        R_ = g.add_node(R)
        OA_= g.add_node(OA)
        OB_= g.add_node(OB)
        A_ = g.add_node(A)
        a_ = g.add_node(A.conj())
        B_ = g.add_node(B)
        b_ = g.add_node(B.conj())

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,B_,4)
        g.connect(R_,1,b_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,OA_,0)
        g.connect(U_,3,A_,1)
        g.connect(U_,4,a_,1)
        g.connect(U_,5,B_,1)
        g.connect(U_,6,b_,1)

        # D tensor
        g.connect(D_,2,OA_,1)
        g.connect(D_,3,A_,3)
        g.connect(D_,4,a_,3)
        g.connect(D_,5,B_,3)
        g.connect(D_,6,b_,3)

        # O tensor
        g.connect(OA_,3,A_,0)
        g.connect(OA_,4,a_,0)
        g.connect(OB_,1,B_,0)
        g.connect(OB_,2,b_,0)
        g.connect(OA_,2,OB_,0)

        # A tensor, A* tensor
        g.connect(A_,4,B_,2)
        g.connect(a_,4,b_,2)

        node_ids = {}
        for name in ["U","L","D","R","OA","OB","A","B"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("matvec_A", ["U","L","D","R","OA","OB","A","B"], [b_], None),
            ("matvec_B", ["U","L","D","R","OA","OB","A","B"], [a_], None),
            ("absorb_into_left_boundary", ["U","L","D","OA","OB","A","B"], [a_,b_], None),
            ("absorb_into_right_boundary", ["U","D","R","OA","OB","A","B"], [a_,b_], None),
            ("absorb_up", ["U", "OA", "OB", "A", "B"], [a_, b_], [L_, R_, D_]),
            ]:
                setattr(self,"two_site_"+name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        s = Subgraph(g)
        for i in xrange(len(g.nodes)):
            s.add_node(i)
        self.two_site_expectation_value = s.merge_all().matrices[0].compile_with_name("two_site_expectation_value","U","L","D","R","OA","OB","A","B")

        del s, g, i

        del U,L,D,R,A,U_,L_,D_,R_,A_,a_,B_,b_

        #@-node:Special 2 body multiplication functions - two sites
        #@+node:Special 2 body multiplication functions - three sites
        g = Graph()

        #@+at
        # Note:
        # 
        # These dimensions are not the actual dimensions;  they are random 
        # prime numbers.
        # 
        # This is done so that index checking can be performed (i.e., to make 
        # sure that I've
        # connected all of the indices properly).  The function that is 
        # produced at the end
        # of the day will not depend on these dimensions at all.
        # 
        #@-at
        #@@c

        #@+at
        # U = Placeholder("U",(6,3,7,5,5,5,5,5,5))
        # L = Placeholder("L",(6,8,5,5))
        # D = Placeholder("D",(3,8,9,5,5,5,5,5,5))
        # R = Placeholder("R",(7,9,5,5))
        # O = Placeholder("O",(3,3,2,2,2,2,2,2))
        # A = Placeholder("A",(2,5,5,5,5))
        # B = Placeholder("B",(2,5,5,5,5))
        # C = Placeholder("C",(2,5,5,5,5))
        #@-at
        #@@c

        U = Placeholder("U",ensure_dimensions_greater_than_one(self.three_site_up_boundary.shape))
        L = Placeholder("L",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        D = Placeholder("D",ensure_dimensions_greater_than_one(self.three_site_down_boundary.shape))
        R = Placeholder("R",ensure_dimensions_greater_than_one(horizontal_boundary_shape))
        OA = Placeholder("OA",ensure_dimensions_greater_than_one(self.three_site_operator_A.shape))
        OB = Placeholder("OB",ensure_dimensions_greater_than_one(self.three_site_operator_B.shape))
        OC = Placeholder("OC",ensure_dimensions_greater_than_one(self.three_site_operator_C.shape))
        A = Placeholder("A",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        B = Placeholder("B",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))
        C = Placeholder("C",ensure_dimensions_greater_than_one(self.system.shape_of_site_matrix))

        U_ = g.add_node(U)
        L_ = g.add_node(L)
        D_ = g.add_node(D)
        R_ = g.add_node(R)
        OA_= g.add_node(OA)
        OB_= g.add_node(OB)
        OC_= g.add_node(OC)
        A_ = g.add_node(A)
        a_ = g.add_node(A.conj())
        B_ = g.add_node(B)
        b_ = g.add_node(B.conj())
        C_ = g.add_node(C)
        c_ = g.add_node(C.conj())

        # L tensor
        g.connect(L_,0,A_,2)
        g.connect(L_,1,a_,2)
        g.connect(L_,2,U_,0)
        g.connect(L_,3,D_,0)

        # R tensor
        g.connect(R_,0,C_,4)
        g.connect(R_,1,c_,4)
        g.connect(R_,2,U_,1)
        g.connect(R_,3,D_,1)

        # U tensor
        g.connect(U_,2,OA_,0)
        g.connect(U_,3,A_,1)
        g.connect(U_,4,a_,1)
        g.connect(U_,5,B_,1)
        g.connect(U_,6,b_,1)
        g.connect(U_,7,C_,1)
        g.connect(U_,8,c_,1)

        # D tensor
        g.connect(D_,2,OA_,1)
        g.connect(D_,3,A_,3)
        g.connect(D_,4,a_,3)
        g.connect(D_,5,B_,3)
        g.connect(D_,6,b_,3)
        g.connect(D_,7,C_,3)
        g.connect(D_,8,c_,3)

        # O tensor
        g.connect(OA_,3,A_,0)
        g.connect(OA_,4,a_,0)
        g.connect(OB_,2,B_,0)
        g.connect(OB_,3,b_,0)
        g.connect(OC_,1,C_,0)
        g.connect(OC_,2,c_,0)
        g.connect(OA_,2,OB_,0)
        g.connect(OB_,1,OC_,0)

        # A tensor, A* tensor
        g.connect(A_,4,B_,2)
        g.connect(a_,4,b_,2)
        g.connect(B_,4,C_,2)
        g.connect(b_,4,c_,2)

        node_ids = {}
        for name in ["U","L","D","R","OA","OB","OC","A","B","C"]:
            node_ids[name] = vars()[name+"_"]

        for name, variables, other_nodes, ordering in [
            ("matvec_B", ["U","L","D","R","OA","OB","OC","A","B","C"], [a_,c_], None),
            ("absorb_up", ["U", "OA", "OB", "OC", "A", "B", "C"], [a_, b_, c_], [L_, R_, D_]),
            ]:
                setattr(self,"three_site_"+name,compile_graph(
                    g,
                    [node_ids[name] for name in variables] + other_nodes,
                    variables,
                    node_ordering=ordering,
                    function_name=name
                    )
                )

        s = Subgraph(g)
        for i in xrange(len(g.nodes)):
            s.add_node(i)
        self.three_site_expectation_value = s.merge_all().matrices[0].compile_with_name("expectation_value","U","L","D","R","OA","OB","OC","A","B","C")

        del s, g, i

        del U,L,D,R,A,U_,L_,D_,R_,A_,a_,B_,b_,C_,c_

        #@-node:Special 2 body multiplication functions - three sites
        #@-others
    #@-node:update_multiplication_functions
    #@+node:absorb_row
    def absorb_row(self,direction,normalizer_left_boundary,normalizer_right_boundary,X):
        assert direction == UP

        self.one_site_up_boundary = self.one_site_absorb_up(self.one_site_up_boundary,self.system.site_matrix
        ).reshape(
            self.one_site_up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
            self.one_site_up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
        )

        self.one_site_up_boundary = multiply_tensor_by_matrix_at_index(self.one_site_up_boundary,X.transpose(),0)
        self.one_site_up_boundary = multiply_tensor_by_matrix_at_index(self.one_site_up_boundary,X.conj().transpose(),1)

        self.two_site_up_boundary = self.two_site_absorb_up(self.two_site_up_boundary,self.two_site_operator_A,self.two_site_operator_B,self.system.site_matrix,self.system.site_matrix,
        ).reshape(
            self.two_site_up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
            self.two_site_up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
            self.two_site_operator_A.shape[0],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
        )

        self.two_site_up_boundary = multiply_tensor_by_matrix_at_index(self.two_site_up_boundary,X.transpose(),0)
        self.two_site_up_boundary = multiply_tensor_by_matrix_at_index(self.two_site_up_boundary,X.conj().transpose(),1)

        self.three_site_up_boundary = self.three_site_absorb_up(
            self.three_site_up_boundary,
            self.three_site_operator_A,
            self.three_site_operator_B,
            self.three_site_operator_C,
            self.system.site_matrix,
            self.system.site_matrix,
            self.system.site_matrix,
        ).reshape(
            self.three_site_up_boundary.shape[0]*self.system.site_matrix.shape[2]**2,
            self.three_site_up_boundary.shape[1]*self.system.site_matrix.shape[4]**2,
            self.three_site_operator_A.shape[0],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
            self.system.site_matrix.shape[3],
        )

        self.three_site_up_boundary = multiply_tensor_by_matrix_at_index(self.three_site_up_boundary,X.transpose(),0)
        self.three_site_up_boundary = multiply_tensor_by_matrix_at_index(self.three_site_up_boundary,X.conj().transpose(),1)

        self.replace_boundaries_with(normalizer_left_boundary,normalizer_right_boundary)

        self.update_multiplication_functions()


    #@-node:absorb_row
    #@+node:absorb_site
    def absorb_site(self,direction):
        assert direction == LEFT or direction == RIGHT
        if self.matvecs_to_ignore > 0:
            self.matvecs_to_ignore -= 1
        if direction == RIGHT and self.ignore_next_right_absorb:
            self.right_boundary = self.one_site_absorb_into_right_boundary(
                self.one_site_up_boundary,
                self.one_site_down_boundary,
                self.right_boundary,
                self.system.site_matrix
                )
            self.ignore_next_right_absorb = False
        elif direction == LEFT and self.ignore_next_left_absorb:
            self.left_boundary = self.one_site_absorb_into_left_boundary(
                self.one_site_up_boundary,
                self.left_boundary,
                self.one_site_down_boundary,
                self.system.site_matrix
                )
            self.ignore_next_left_absorb = False
        elif self.number_of_active_sites == 3:
            assert self.B is None
            if direction == LEFT:
                self.left_boundary = self.two_site_absorb_into_left_boundary(
                    self.two_site_up_boundary,
                    self.left_boundary,
                    self.two_site_down_boundary,
                    self.two_site_operator_A,
                    self.two_site_operator_B,
                    self.A,
                    self.system.site_matrix
                    )
                self.A = None
                self.B = self.C
            else:
                self.right_boundary = self.two_site_absorb_into_right_boundary(
                    self.two_site_up_boundary,
                    self.two_site_down_boundary,
                    self.right_boundary,
                    self.two_site_operator_A,
                    self.two_site_operator_B,
                    self.system.site_matrix,
                    self.C
                    )
                self.B = None
            del self.C
            self.number_of_active_sites = 2
        elif self.number_of_active_sites == 1:
            if direction == LEFT:
                self.A = self.system.site_matrix.copy()
                self.B = None
            else:
                self.A = None
                self.B = self.system.site_matrix.copy()
            self.number_of_active_sites = 2
        else:
            assert self.number_of_active_sites == 2
            if self.A is None:
                assert self.B is not None
                if direction == LEFT:
                    self.A = self.system.site_matrix.copy()
                    self.C = self.B
                    self.B = None
                    self.number_of_active_sites = 3
                else:
                    self.right_boundary = self.two_site_absorb_into_right_boundary(
                        self.two_site_up_boundary,
                        self.two_site_down_boundary,
                        self.right_boundary,
                        self.two_site_operator_A,
                        self.two_site_operator_B,
                        self.system.site_matrix,
                        self.B
                    )
                    self.number_of_active_sites = 1
                    del self.A
                    del self.B
            else:
                assert self.B is None
                if direction == RIGHT:
                    self.C = self.system.site_matrix.copy()
                    self.number_of_active_sites = 3
                else:
                    self.left_boundary = self.two_site_absorb_into_left_boundary(
                        self.two_site_up_boundary,
                        self.left_boundary,
                        self.two_site_down_boundary,
                        self.two_site_operator_A,
                        self.two_site_operator_B,
                        self.A,
                        self.system.site_matrix
                    )
                    self.number_of_active_sites = 1
                    del self.A
                    del self.B

    #@-node:absorb_site
    #@+node:absorb_unitary
    def absorb_unitary(self,unitary):

        if self.number_of_active_sites == 1:
            self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
            self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
            self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
            self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)
        elif self.number_of_active_sites == 3:
            self.A = multiply_tensor_by_matrix_at_index(self.A,unitary,4)
            self.C = multiply_tensor_by_matrix_at_index(self.C,unitary.conj(),2)
        else:
            assert self.number_of_active_sites == 2
            if self.A is None:
                self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary,0)
                self.left_boundary = multiply_tensor_by_matrix_at_index(self.left_boundary,unitary.conj(),1)
                self.B = multiply_tensor_by_matrix_at_index(self.B,unitary.conj(),2)
            else:
                assert self.B is None
                self.A = multiply_tensor_by_matrix_at_index(self.A,unitary,4)
                self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary.conj(),0)
                self.right_boundary = multiply_tensor_by_matrix_at_index(self.right_boundary,unitary,1)
        horizontal_boundary_shape = list(self.left_boundary.shape)
        horizontal_boundary_shape[0] = horizontal_boundary_shape[1] = self.system.shape_of_site_matrix[2]
        self.update_multiplication_functions(tuple(horizontal_boundary_shape))


    #@-node:absorb_unitary
    #@+node:replace_boundaries_with
    def replace_boundaries_with(self,normalizer_left_boundary,normalizer_right_boundary):
        self.number_of_active_sites = 1

        self.ignore_next_left_absorb = self.ignore_first_left
        self.ignore_next_right_absorb= self.ignore_first_right
        self.matvecs_to_ignore = 2 if self.ignore_first_two_matvecs else 0

        horizontal_boundary_shape = copy(normalizer_left_boundary.shape)
        horizontal_boundary_shape[-1] *= 2

        self.left_boundary = multiply.outer(normalizer_left_boundary,array([1,0])).reshape(horizontal_boundary_shape)
        self.right_boundary = multiply.outer(normalizer_right_boundary,array([0,1])).reshape(horizontal_boundary_shape)
    #@-node:replace_boundaries_with
    #@-others
#@-node:class OldSpinCouplingTerm
#@-node:Two Dimensions
#@-others
#@-node:@file InfiniteOpenProductState.py
#@-leo
