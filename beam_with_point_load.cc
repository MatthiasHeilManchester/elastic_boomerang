//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC// Copyright (C) 2006-2024 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
//Driver function for a simple beam proble

//OOMPH-LIB includes
#include "generic.h"
#include "beam.h"
#include "meshes/one_d_lagrangian_mesh.h"

using namespace std;

using namespace oomph;


//========= start_of_point_load_wrapper==============================
/// Class to impose point load to (wrapped) beam element
//=====================================================================
template<class ELEMENT> 
class BeamPointLoadElement : public virtual ELEMENT
{

public:

 /// Constructor
 BeamPointLoadElement()
  {
  }
 
 /// Destructor (empty)
 ~BeamPointLoadElement(){}


 /// Set local coordinate and magnitude of point load
 // include "bending" term too and tidy up terminology);
 // also pass pointers rather than actual values.
 void setup(const Vector<double>& s_point_load,
            const Vector<double>& point_load)
  {
   S_point_load=s_point_load;
   Point_load=point_load;
  }
 
 
 /// Add the element's contribution to its residual vector (wrapper)
 void fill_in_contribution_to_residuals(Vector<double> &residuals)
  {
   //Call the generic residuals function
   ELEMENT::fill_in_contribution_to_residuals(residuals);

   // Add point load contribution
   fill_in_point_load_contribution_to_residuals(residuals);
  }

 

 /// Add the element's contribution to its residual vector and
 /// element Jacobian matrix (wrapper)
 void fill_in_contribution_to_jacobian(Vector<double> &residuals,
                                       DenseMatrix<double> &jacobian)
  {
   //Call the generic routine
   ELEMENT::fill_in_contribution_to_jacobian(residuals,
                                             jacobian);

   // Add point load contribution (doesn't (currently) depend
   // on displacements hierher think about this in fsi context)
   fill_in_point_load_contribution_to_residuals(residuals);
  }
 

private:

 
 /// Add the point load contribution to the residual vector
 void fill_in_point_load_contribution_to_residuals(Vector<double> &residuals)
  {
   // No further action
   if (S_point_load.size()==0) return;
   
   // Set the dimension of the global coordinates
   const unsigned n_dim = this->Undeformed_beam_pt->ndim();
   
   // Set the number of lagrangian coordinates
   const unsigned n_lagrangian = this->Undeformed_beam_pt->nlagrangian();
   
   // Find out how many nodes there are
   const unsigned n_node = this->nnode();
   
   // Find out how many positional dofs there are
   const unsigned n_position_type = this->nnodal_position_type();
   
   // Integer to store the local equation number
   int local_eqn = 0;
   
   // Set up memory for the shape functions and derivs.
   // hierher do we need all of these?
   
   // # of nodes, # of positional dofs
   Shape psi(n_node, n_position_type);
   
   // # of nodes, # of positional dofs, # of lagrangian coords (for deriv)
   DShape dpsidxi(n_node, n_position_type, n_lagrangian);
   
   // # of nodes, # of positional dofs, # of derivs)
   DShape d2psidxi(n_node, n_position_type, n_lagrangian);
   
   // Get shape functions and derivatives
   double J = this->d2shape_lagrangian(S_point_load, psi, dpsidxi, d2psidxi);
   
   
   // Loop over the number of nodes
   for (unsigned n = 0; n < n_node; n++)
    {
     // Loop over the type of degree of freedom
     for (unsigned k = 0; k < n_position_type; k++)
      {
       // Loop over the coordinate directions
       for (unsigned i = 0; i < n_dim; i++)
        {
         // Find the equation number
         local_eqn = this->position_local_eqn(n, k, i);
         
         // If it's not a boundary condition
         if (local_eqn >= 0)
          {
           // Point load
           residuals[local_eqn] += Point_load[i]*psi(n,k);

           oomph_info << "adding "  << Point_load[i]*psi(n,k) << std::endl;
          }
        }
      }
    }
   
  }
 
 /// Local coordinates of point at which point load is applied
 Vector<double> S_point_load;

 /// Magnitude of point load
 Vector<double> Point_load;

 };



//=======================================================================
/// Face geometry for element is the same as that for the underlying
/// wrapped element
//=======================================================================
 template<class ELEMENT>
 class FaceGeometry<BeamPointLoadElement<ELEMENT> > 
  : public virtual FaceGeometry<ELEMENT>
 {
 public:
  FaceGeometry() : FaceGeometry<ELEMENT>() {}
 };


//=======================================================================
/// Face geometry of the Face Geometry for element is the same as 
/// that for the underlying wrapped element
//=======================================================================
 template<class ELEMENT>
 class FaceGeometry<FaceGeometry<BeamPointLoadElement<ELEMENT> > >
  : public virtual FaceGeometry<FaceGeometry<ELEMENT> >
 {
 public:
  FaceGeometry() : FaceGeometry<FaceGeometry<ELEMENT> >() {}
 };





////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////




//========start_of_namespace========================
/// Namespace for physical parameters
//==================================================
namespace Global_Physical_Variables
{
 /// Non-dimensional thickness
 double H;

 /// 2nd Piola Kirchhoff pre-stress
 double Sigma0;

 /// Pressure load
 double P_ext;

 /// Load function: Apply a constant external pressure to the beam
 void load(const Vector<double>& xi, const Vector<double> &x,
           const Vector<double>& N, Vector<double>& load)
 {
  for(unsigned i=0;i<2;i++) {load[i] = -P_ext*N[i];}
 }


 /// Point load
 Vector<double> Point_load{0.0,0.0}; // {1.0,1.0};

 /// Locatino of point load
 Vector<double> S_point_load{0.5};



} // end of namespace

//======start_of_problem_class==========================================
/// Beam problem object
//======================================================================
class ElasticBeamProblem : public Problem
{
public:
 
 /// Constructor: The arguments are the number of elements, 
 /// the length of domain
 ElasticBeamProblem(const unsigned &n_elem, const double &length);
 
 /// Conduct a parameter study
 void parameter_study();
 
 /// Return pointer to the mesh
 OneDLagrangianMesh<BeamPointLoadElement<HermiteBeamElement>>* mesh_pt() 
  {return dynamic_cast<OneDLagrangianMesh<BeamPointLoadElement<HermiteBeamElement>>*>
    (Problem::mesh_pt());}

 /// No actions need to be performed after a solve
 void actions_after_newton_solve() {}

 /// No actions need to be performed before a solve
 void actions_before_newton_solve() {}

private:

 /// Pointer to the node whose displacement is documented
 Node* Doc_node_pt;

 /// Length of domain (in terms of the Lagrangian coordinates)
 double Length;

 /// Pointer to geometric object that represents the beam's undeformed shape
 GeomObject* Undef_beam_pt;

}; // end of problem class


//=============start_of_constructor=====================================
/// Constructor for elastic beam problem
//======================================================================
ElasticBeamProblem::ElasticBeamProblem(const unsigned &n_elem,
                                       const double &length) : Length(length)
{
 // Set the undeformed beam to be a straight line at y=0
 Undef_beam_pt=new StraightLine(0.0); 

 // Create the (Lagrangian!) mesh, using the geometric object
 // Undef_beam_pt to specify the initial (Eulerian) position of the
 // nodes.
 Problem::mesh_pt() = 
  new OneDLagrangianMesh<BeamPointLoadElement<HermiteBeamElement> >(n_elem,length,Undef_beam_pt);

 // Set the boundary conditions: Each end of the beam is fixed in space
 // Loop over the boundaries (ends of the beam)
 for(unsigned b=0;b<2;b++)
  {
   // Pin displacements in both x and y directions
   // [Note: The mesh_pt() function has been overloaded
   //  to return a pointer to the actual mesh, rather than
   //  a pointer to the Mesh base class. The current mesh is derived
   //  from the SolidMesh class. In such meshes, all access functions
   //  to the nodes, such as boundary_node_pt(...), are overloaded
   //  to return pointers to SolidNodes (whose position can be
   //  pinned) rather than "normal" Nodes.]
   mesh_pt()->boundary_node_pt(b,0)->pin_position(0); 
   mesh_pt()->boundary_node_pt(b,0)->pin_position(1); 
  }
 
 //Find number of elements in the mesh
 unsigned n_element = mesh_pt()->nelement();
 
 //Loop over the elements to set physical parameters etc.
 for(unsigned e=0;e<n_element;e++)
  {
   // Upcast to the specific element type
   BeamPointLoadElement<HermiteBeamElement> *elem_pt = 
    dynamic_cast<BeamPointLoadElement<HermiteBeamElement>*>(mesh_pt()->element_pt(e));
   
   // Set physical parameters for each element:
   elem_pt->sigma0_pt() = &Global_Physical_Variables::Sigma0;
   elem_pt->h_pt() = &Global_Physical_Variables::H;

   // Set the load Vector for each element
   elem_pt->load_vector_fct_pt() = &Global_Physical_Variables::load;

   // Set the undeformed shape for each element
   elem_pt->undeformed_beam_pt() = Undef_beam_pt;
  } // end of loop over elements

 // Choose node at which displacement is documented (halfway along -- provided
 // we have an odd number of nodes; complain if this is not the
 // case because the comparison with the exact solution will be wrong 
 // otherwise!)
 unsigned n_nod=mesh_pt()->nnode();
 if (n_nod%2!=1)
  {
   cout << "Warning: Even number of nodes " << n_nod << std::endl;
   cout << "Comparison with exact solution will be misleading..." << std::endl;
  }
 Doc_node_pt=mesh_pt()->node_pt((n_nod+1)/2-1);


 // Apply point load to middle element
 unsigned e_middle=unsigned(double(n_element)*0.5);
 BeamPointLoadElement<HermiteBeamElement>* middle_elem_pt =
  dynamic_cast<BeamPointLoadElement<HermiteBeamElement>*>(
   mesh_pt()->element_pt(e_middle));

 // hierher rewrite to pointers!
 middle_elem_pt->setup(Global_Physical_Variables::S_point_load,
                       Global_Physical_Variables::Point_load);
                        
 // Assign the global and local equation numbers
 cout << "# of dofs " << assign_eqn_numbers() << std::endl;

} // end of constructor


//=======start_of_parameter_study==========================================
/// Solver loop to perform parameter study
//=========================================================================
void ElasticBeamProblem::parameter_study()
{
 // Over-ride the default maximum value for the residuals
 Problem::Max_residuals = 1.0e10;
 
 // Set the increments in control parameters
 double pext_increment = 0.001;
 
 // Set initial values for control parameters 
 Global_Physical_Variables::P_ext = 0.0 - pext_increment;
 
 // Create label for output
 DocInfo doc_info;
 
 // Set output directory -- this function checks if the output
 // directory exists and issues a warning if it doesn't.
 doc_info.set_directory("RESLT");
 
 // Open a trace file
 ofstream trace("RESLT/trace_beam.dat");
 
 // Write a header for the trace file
 trace << 
  "VARIABLES=\"p_e_x_t\",\"d\"" << 
  ", \"p_e_x_t_(_e_x_a_c_t_)\"" << std::endl;
 
 // Output file stream used for writing results
 ofstream file;
 // String used for the filename
 char filename[100]; 

 // Loop over parameter increments
 unsigned nstep=10;
 for(unsigned i=1;i<=nstep;i++)
  {
   // Increment pressure
   Global_Physical_Variables::P_ext += pext_increment;

   // Solve the system
   newton_solve();
    
   // Calculate exact solution for `string under tension' (applicable for
   // small wall thickness and pinned ends)

   // The tangent of the angle beta
   double tanbeta =-2.0*Doc_node_pt->x(1)/Length;

   double exact_pressure = 0.0;
   //If the beam has deformed, calculate the pressure required
   if(tanbeta!=0)
    {
      
      //Calculate the opening angle alpha
      double alpha = 2.0*atan(2.0*tanbeta/(1.0-tanbeta*tanbeta));

      // Jump back onto the main branch if alpha>180 degrees
      if (alpha<0) alpha+=2.0*MathematicalConstants::Pi;

     // Green strain:
     double gamma=0.5*(0.25*alpha*alpha/(sin(0.5*alpha)*sin(0.5*alpha))-1.0);

     //Calculate the exact pressure
     exact_pressure=Global_Physical_Variables::H*
      (Global_Physical_Variables::Sigma0+gamma)*alpha/Length;
    } 
   
   // Document the solution
   sprintf(filename,"RESLT/beam%i.dat",i);
   file.open(filename);
   mesh_pt()->output(file,5);
   file.close();
   
   // Write trace file: Pressure, displacement and exact solution
   // (for string under tension)
   trace << Global_Physical_Variables::P_ext  << " " 
         << abs(Doc_node_pt->x(1))
         << " " << exact_pressure 
         << std::endl;
  }
 
} // end of parameter study

//========start_of_main================================================
/// Driver for beam (string under tension) test problem 
//=====================================================================
int main()
{

 // Set the non-dimensional thickness 
 Global_Physical_Variables::H=0.01; 
 
 // Set the 2nd Piola Kirchhoff prestress
 Global_Physical_Variables::Sigma0=0.1; 
 
 // Set the length of domain
 double L = 10.0;

 // Number of elements (choose an even number if you want the control point 
 // to be located at the centre of the beam)
 unsigned n_element = 10;

 // Construst the problem
 ElasticBeamProblem problem(n_element,L);

 // Check that we're ready to go:
 cout << "\n\n\nProblem self-test ";
 if (problem.self_test()==0) 
  {
   cout << "passed: Problem can be solved." << std::endl;
  }
 else 
  {
   throw OomphLibError("Self test failed",
                       OOMPH_CURRENT_FUNCTION,
                       OOMPH_EXCEPTION_LOCATION);
  }

 // Conduct parameter study
 problem.parameter_study();

} // end of main

