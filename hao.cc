// LIC// ====================================================================
// LIC// This file forms part of oomph-lib, the object-oriented,
// LIC// multi-physics finite-element library, available
// LIC// at http://www.oomph-lib.org.
// LIC//
// LIC// Copyright (C) 2006-2023 Matthias Heil and Andrew Hazel
// LIC//
// LIC// This library is free software; you can redistribute it and/or
// LIC// modify it under the terms of the GNU Lesser General Public
// LIC// License as published by the Free Software Foundation; either
// LIC// version 2.1 of the License, or (at your option) any later version.
// LIC//
// LIC// This library is distributed in the hope that it will be useful,
// LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
// LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// LIC// Lesser General Public License for more details.
// LIC//
// LIC// You should have received a copy of the GNU Lesser General Public
// LIC// License along with this library; if not, write to the Free Software
// LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// LIC// 02110-1301  USA.
// LIC//
// LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
// LIC//
// LIC//====================================================================
// Driver function for a simple beam problem

// OOMPH-LIB includes
#include "generic.h"
#include "beam.h"
#include "meshes/one_d_lagrangian_mesh.h"

using namespace std;
using namespace oomph;

//========start_of_namespace========================
/// Namespace for physical parameters
//==================================================
namespace Global_Physical_Variables
{
  /// Non-dimensional thickness
  double H = 0.0;

  /// Non-dimensional coefficient (FSI)
  double Q = 0.0;

  /// Angle between the two arms of the beam
  double Alpha = 0.0;

} // namespace Global_Physical_Variables


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


//=========================================================================
/// RigidBodyElement
//=========================================================================
class RigidBodyElement : public GeneralisedElement
{
public:
  /// Constructor: Pass initial values for rigid body parameters (pinned
  /// by default)
  RigidBodyElement(const double& V,
                   const double& U0,
                   const double& Theta_eq,
                   const double& X0,
                   const double& Y0)
  {
    // Create internal data which contains the "rigid body" parameters
    for (unsigned i = 0; i < 5; i++)
    {
      // Create data: One value, no timedependence, free by default
      add_internal_data(new Data(1));
    }

    // Give them a value:
    internal_data_pt(0)->set_value(0, V);
    internal_data_pt(1)->set_value(0, U0);
    internal_data_pt(2)->set_value(0, Theta_eq);

    // These are just initial values so pin
    internal_data_pt(3)->set_value(0, X0);
    internal_data_pt(3)->pin(0);
    internal_data_pt(4)->set_value(0, Y0);
    internal_data_pt(4)->pin(0);
  }


  /// Function that returns the Vector of pointers to the "rigid body"
  /// parameters
  Vector<Data*> rigid_body_parameters()
  {
    Vector<Data*> tmp_pt(5);
    for (unsigned i = 0; i < 5; i++)
    {
      tmp_pt[i] = internal_data_pt(i);
    }
    return tmp_pt;
  }


  /// Helper function to compute the meaningful parameter values
  /// from enumerated data
  void get_parameters(
    double& V, double& U0, double& Theta_eq, double& X0, double& Y0)
  {
    V = internal_data_pt(0)->value(0);
    U0 = internal_data_pt(1)->value(0);
    Theta_eq = internal_data_pt(2)->value(0);
    X0 = internal_data_pt(3)->value(0);
    Y0 = internal_data_pt(4)->value(0);
  }



 // hierher kill
 
  /// Note that this is the helper function to compute the meaningful parameter
  /// values for the second arm of the beam (rotation angle now is
  /// Theta_eq-Alpha)
  void get_parameters_second_arm(
    double& V, double& U0, double& Theta_eq, double& X0, double& Y0)
  {
    V = internal_data_pt(0)->value(0);
    U0 = internal_data_pt(1)->value(0);
    Theta_eq = internal_data_pt(2)->value(0) - Global_Physical_Variables::Alpha;
    X0 = internal_data_pt(3)->value(0);
    Y0 = internal_data_pt(4)->value(0);
  }


  /// Pass pointer to the Mesh of HaoHermiteBeamElements
  /// and add their unknowns to be external data for this element
  void set_pointer_to_beam_mesh(SolidMesh* beam_mesh_pt)
  {
    // Store the pointer for future reference
    Beam_mesh_pt = beam_mesh_pt;

    // Loop over the nodes in the mesh and add them as external Data
    // because they affect the traction and therefore the total drag
    // and torque on the object.
    unsigned nnode = beam_mesh_pt->nnode();
    for (unsigned j = 0; j < nnode; j++)
    {
      add_external_data(beam_mesh_pt->node_pt(j)->variable_position_pt());
    }
  }


  /// Pass pointer to the Mesh of HaoHermiteBeamElements
  /// and add their unknowns to be external data for this element
  void set_pointer_to_beam_second_arm_mesh(SolidMesh* beam_mesh_second_arm_pt)
  {
    // Store the pointer for future reference
    Beam_mesh_second_arm_pt = beam_mesh_second_arm_pt;

    // Loop over the nodes in the mesh and add them as external Data
    // because they affect the traction and therefore the total drag
    // and torque on the object.
    unsigned nnode = beam_mesh_second_arm_pt->nnode();
    for (unsigned j = 0; j < nnode; j++)
    {
      add_external_data(
        beam_mesh_second_arm_pt->node_pt(j)->variable_position_pt());
    }
  }


 // kill all references to type
  /// Compute the beam's centre of mass (Type=0: first arm, Type=1: second arm.)
  void compute_centre_of_mass(Vector<double>& r_centre, const unsigned& Type);


  /// Compute the total drag and torque on the entire beam structure according
  /// to slender body theory (Type=0: first arm, Type=1: second arm.)
  void compute_drag_and_torque(Vector<double>& total_drag,
                               double& total_torque,
                               const unsigned Type);


  /// Output the total drag and torque on the entire beam structure (Type=0:
  /// first arm, Type=1: second arm.)
  void output(std::ostream& outfile, const unsigned Type)
  {
    Vector<double> total_drag(2);
    double total_torque = 0.0;

    // Compute the total drag and torque on the entire beam structure
    compute_drag_and_torque(total_drag, total_torque, Type);

    // Output Theta_eq, total drag and torque
    outfile << internal_data_pt(2)->value(0) << "  ";
    outfile << total_drag[0] << "  ";
    outfile << total_drag[1] << "  ";
    outfile << total_torque << std::endl;
  }


protected:
  // Fill in contribution to residuals
  void fill_in_contribution_to_residuals(Vector<double>& residuals)
  {
    oomph_info << "ndof in element: " << residuals.size() << std::endl;
    // Get current total drag and torque (Type=0: first arm, Type=1: second
    // arm.)
   
    // hierher only call this function once but but the compute_drag_and_torque
    // fct should loop both beam meshes! 

    Vector<double> total_drag_0(2);
    double total_torque_0 = 0.0;
    compute_drag_and_torque(total_drag_0, total_torque_0, 0);


    // Get current total drag and torque (Type=0: first arm, Type=1: second
    // arm.)
    Vector<double> total_drag(2);
    double total_torque = 0.0;
    compute_drag_and_torque(total_drag, total_torque, 1);


    unsigned n_internal = ninternal_data();
    for (unsigned i = 0; i < n_internal; i++)
    {
      // Get the local equation number of the zeroth dof
      // associated with this internal Data object
      unsigned j = 0;
      int eqn_number = internal_local_eqn(i, j);

      // Is it an actual dof
      if (eqn_number >= 0)
      {
        if (i == 0)
        {
          // Eqn for V:
          residuals[eqn_number] = total_drag_0[0] + total_drag[0];
          // internal_data_pt(i)->value(j)-
          // Global_Physical_Variables::V;
        }
        else if (i == 1)
        {
          // Eqn for U0:
          residuals[eqn_number] = total_drag_0[1] + total_drag[1];
          // internal_data_pt(i)->value(j)-
          // Global_Physical_Variables::U0;
        }
        else if (i == 2)
        {
          // Eqn for Theta_eq:
          residuals[eqn_number] = total_torque_0 + total_torque;
          // internal_data_pt(i)->value(j)-
          // Global_Physical_Variables::Theta_eq;
        }
        else
        {
          oomph_info << "Never get here\n";
          abort();
        }

        std::cout << "internal data " << i << " is not pinned\n";
      }
      else
      {
        std::cout << "internal data " << i << " is pinned\n";
      }
    }
  }

private:

  /// Pointer to the Mesh of HaoHermiteBeamElements (first arm)
  SolidMesh* Beam_mesh_first_arm_pt; // hierher fix this.


  /// Pointer to the Mesh of HaoHermiteBeamElements (second arm)
  SolidMesh* Beam_mesh_second_arm_pt;
};


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


//=====================================================================
/// Upgraded Hermite Beam Element to incorporate slender body traction
//=====================================================================
class HaoHermiteBeamElement : public virtual HermiteBeamElement
{
 
public:
 
 /// Constructor: Initialise private member data
 HaoHermiteBeamElement() : Rigid_body_element_pt(0),
                           Q_pt(0), Theta_initial_pt(0)
  {}
 
 
 /// Pass pointer to RigidBodyElement that contains the rigid body parameters
 void set_pointer_to_rigid_body_element(
    RigidBodyElement* rigid_body_element_pt)
  {
    // Store the pointer for future reference
    Rigid_body_element_pt = rigid_body_element_pt;

    // Get the rigid body parameters
    Vector<Data*> rigid_body_data_pt =
      Rigid_body_element_pt->rigid_body_parameters();

#ifdef PARANOID
    if (rigid_body_data_pt.size() != 5)
    {
      std::ostringstream error_message;
      error_message << "rigid_body_data_pt should have size 5, not "
                    << rigid_body_data_pt.size() << std::endl;

      // loop over all entries
      for (unsigned i = 0; i < 5; i++)
      {
        if (rigid_body_data_pt[i]->nvalue() != 1)
        {
          error_message << "rigid_body_data_pt[" << i
                        << "] should have 1 value, not "
                        << rigid_body_data_pt[i]->nvalue() << std::endl;
        }
      }

      throw OomphLibError(
        error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }
#endif

    // Add the rigid body parameters as the external data for this element
    for (unsigned i = 0; i < 5; i++)
    {
      add_external_data(rigid_body_data_pt[i]);
    }
  }


  /// Pointer to non-dimensional coefficient (FSI)
  double*& q_pt()
  {
    return Q_pt;
  }


 /// Pointer to initial angle
 void theta_initial_pt(const double* theta_initial_pt)
  {
   Theta_initial_pt=theta_initial_pt;
  }

 /// Initial angle
 void theta_initial() const
  {
   if (Theta_initial_pt==0)
    {
     return 0.0;
    }
   else
    {
     return *Theta_initial_pt;
    }
  }


 // hierher kill
  /// select first arm of the beam
  void select_first_arm()
  {
    Type = 0;
  }


 // hierher kill
  /// select second arm of the beam
  void select_second_arm()
  {
    Type = 1;
  }

  /// Compute the element's contribution to the (\int r ds) and length of beam
  void compute_contribution_to_int_r_and_length(Vector<double>& int_r,
                                                double& length)
  {
#ifdef PARANOID
    if (int_r.size() != 2)
    {
      std::ostringstream error_message;
      error_message << "int_r should have size 2, not " << int_r.size()
                    << std::endl;

      throw OomphLibError(
        error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }
#endif

    // Initialise
    int_r[0] = 0.0;
    int_r[1] = 0.0;
    length = 0.0;

    // Local coordinate (1D!)
    Vector<double> s(1);

    // Set # of integration points
    const unsigned n_intpt = integral_pt()->nweight();

    // Loop over the integration points
    for (unsigned ipt = 0; ipt < n_intpt; ipt++)
    {
      // Get the integral weight
      double w = integral_pt()->weight(ipt);

      // Return local coordinate s[j]  of i-th integration point.
      unsigned j = 0;
      s[j] = integral_pt()->knot(ipt, j);

      // Get position vector to and non-unit tangent vector on wall:
      // dr/ds. NOTE: This is before we apply the rigid body motion!
      // so in terms of the write-up the position vector is R_0
      Vector<double> R_0(2);
      Vector<double> drds(2);
      get_non_unit_tangent(s, R_0, drds);

      // Jacobian of mapping between local and global coordinates
      double J = sqrt(drds[0] * drds[0] + drds[1] * drds[1]);

      // Premultiply the weights and the Jacobian
      double W = w * J;

      // Translate rigid body parameters into meaningful variables
      // (Type=0: first arm, Type=1: second arm.)
      double V = 0.0;
      double U0 = 0.0;
      double Theta_eq = 0.0;
      double X0 = 0.0;
      double Y0 = 0.0;
      if (Type == 0)
      {
        Rigid_body_element_pt->get_parameters(V, U0, Theta_eq, X0, Y0);
      }
      else
      {
        Rigid_body_element_pt->get_parameters_second_arm(
          V, U0, Theta_eq, X0, Y0);
      }

      // Note that we're looking for an pseudo "equilibrium position"
      // where the angle (and the traction!) remain constant while
      // the beam still moves as a rigid body!
      double t = 0.0;


      // hierher use Theta_initial everywhere whenever you're processing Theta_eq

      // Apply rigid body translation and rotation to get the actual
      // shape of the deformed body in the fluid
      Vector<double> R(2);
      R[0] = cos(Theta_eq-theta_initial()) * R_0[0] - sin(Theta_eq-theta_initial()) * R_0[1] + 0.5 * V * t * t +
             U0 * t + X0;
      R[1] = sin(Theta_eq-theta_initial()) * R_0[0] + cos(Theta_eq-theta_initial()) * R_0[1] + V * t + Y0;

      // Add 'em.
      length += W;
      int_r[0] += R[0] * W;
      int_r[1] += R[1] * W;
    }
  }


  /// Compute the slender body traction acting on the actual beam onto the
  /// element at local coordinate s
  void compute_slender_body_traction_on_actual_beam(const Vector<double>& s,
                                                    Vector<double>& traction)
  {
#ifdef PARANOID
    if (traction.size() != 2)
    {
      std::ostringstream error_message;
      error_message << "traction should have size 2, not " << traction.size()
                    << std::endl;

      throw OomphLibError(
        error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }
#endif

    // Get the Eulerian position and the unit normal.
    // NOTE: This is before we apply the rigid body motion!
    // so in terms of the write-up the position vector is R_0 and N_0
    Vector<double> R_0(2);
    Vector<double> N_0(2);
    get_normal(s, R_0, N_0);

    // Translate rigid body parameters into meaningful variables
    // (Type=0: first arm, Type=1: second arm.)
    double V = 0.0;
    double U0 = 0.0;
    double Theta_eq = 0.0;
    double X0 = 0.0;
    double Y0 = 0.0;
    if (Type == 0)
    {
      Rigid_body_element_pt->get_parameters(V, U0, Theta_eq, X0, Y0);
    }
    else
    {
      Rigid_body_element_pt->get_parameters_second_arm(V, U0, Theta_eq, X0, Y0);
    }

    // Note that we're looking for an pseudo "equilibrium position"
    // where the angle (and the traction!) remain constant while
    // the beam still moves as a rigid body!
    double t = 0.0;

    // Compute R which is after translation and rotation
    Vector<double> R(2);
    R[0] = cos(Theta_eq-theta_initial()) * R_0[0] - sin(Theta_eq-theta_initial()) * R_0[1] + 0.5 * V * t * t +
           U0 * t + X0;
    R[1] = sin(Theta_eq-theta_initial()) * R_0[0] + cos(Theta_eq-theta_initial()) * R_0[1] + V * t + Y0;

    // Compute normal N which is after translation and rotation
    Vector<double> N(2);
    N[0] = cos(Theta_eq-theta_initial()) * N_0[0] - sin(Theta_eq-theta_initial()) * N_0[1];
    N[1] = sin(Theta_eq-theta_initial()) * N_0[0] + cos(Theta_eq-theta_initial()) * N_0[1];

    // Compute the traction onto the element at local coordinate s
    traction[0] = 0.5 * (V * t - R[1] + U0) * N[1] * N[1] -
                  0.5 * N[1] * N[0] * V - V * t - U0 + R[1];

    traction[1] =
      0.5 * V * N[0] * N[0] - 0.5 * N[1] * (V * t - R[1] + U0) * N[0] - V;
  }


  /// Compute the slender body traction acting on the beam in the reference
  /// configuration (i.e. without rigid body motion!) at local coordinate s
  void compute_slender_body_traction_on_beam_in_reference_configuration(
    const Vector<double>& s, Vector<double>& traction_0)
  {
#ifdef PARANOID
    if (traction_0.size() != 2)
    {
      std::ostringstream error_message;
      error_message << "traction_0 should have size 2, not "
                    << traction_0.size() << std::endl;

      throw OomphLibError(
        error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }
#endif

    // Translate rigid body parameters into meaningful variables
    // (Type=0: first arm, Type=1: second arm.)
    double V = 0.0;
    double U0 = 0.0;
    double Theta_eq = 0.0;
    double X0 = 0.0;
    double Y0 = 0.0;
    if (Type == 0)
    {
      Rigid_body_element_pt->get_parameters(V, U0, Theta_eq, X0, Y0);
    }
    else
    {
      Rigid_body_element_pt->get_parameters_second_arm(V, U0, Theta_eq, X0, Y0);
    }

    // Compute the slender body traction acting on the actual beam onto the
    // element at local coordinate s
    Vector<double> traction(2);
    compute_slender_body_traction_on_actual_beam(s, traction);

    // Rotate the traction from the actual beam back to the reference
    // configuration.
    traction_0[0] =  traction[0] * cos(Theta_eq-theta_initial()) + traction[1] * sin(Theta_eq-theta_initial());
    traction_0[1] = -traction[0] * sin(Theta_eq-theta_initial()) + traction[1] * cos(Theta_eq-theta_initial());
  }


  // overloaded load_vector to apply the computed traction_0 (i.e. the
  // traction acting on the beam before its rigid body motion is applied)
  // including the non-dimensional coefficient Q (FSI)
  void load_vector(const unsigned& intpt,
                   const Vector<double>& xi,
                   const Vector<double>& x,
                   const Vector<double>& N,
                   Vector<double>& load)
  {
    /// Return local coordinate s[j] at the specified integration point.
    Vector<double> s(1);
    unsigned j = 0;
    s[j] = integral_pt()->knot(intpt, j);

    compute_slender_body_traction_on_beam_in_reference_configuration(s, load);
    load[0] = *(q_pt()) * load[0];
    load[1] = *(q_pt()) * load[1];
  }


  // Compute the element's contribution to the total drag and torque on
  // the entire beam structure according to slender body theory
  void compute_contribution_to_drag_and_torque(Vector<double>& drag,
                                               double& torque)
  {
#ifdef PARANOID
    if (drag.size() != 2)
    {
      std::ostringstream error_message;
      error_message << "drag should have size 2, not " << drag.size()
                    << std::endl;

      throw OomphLibError(
        error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
    }
#endif

    // Initialise
    drag[0] = 0.0;
    drag[1] = 0.0;
    torque = 0.0;

    // Compute the beam's positon of centre of mass
    Vector<double> r_centre(2);
    Rigid_body_element_pt->compute_centre_of_mass(r_centre, Type);

    // Local coordinate (1D!)
    Vector<double> s(1);

    // Set # of integration points
    const unsigned n_intpt = integral_pt()->nweight();

    // Translate rigid body parameters into meaningful variables
    // (Type=0: first arm, Type=1: second arm.)
    double V = 0.0;
    double U0 = 0.0;
    double Theta_eq = 0.0;
    double X0 = 0.0;
    double Y0 = 0.0;
    if (Type == 0)
    {
      Rigid_body_element_pt->get_parameters(V, U0, Theta_eq, X0, Y0);
    }
    else
    {
      Rigid_body_element_pt->get_parameters_second_arm(V, U0, Theta_eq, X0, Y0);
    }

    // Note that we're looking for an pseudo "equilibrium position"
    // where the angle (and the traction!) remain constant while
    // the beam still moves as a rigid body!
    double t = 0.0;

    // Loop over the integration points
    for (unsigned ipt = 0; ipt < n_intpt; ipt++)
    {
      // Get the integral weight
      double w = integral_pt()->weight(ipt);

      /// Return local coordinate s[j] of i-th integration point.
      unsigned j = 0;
      s[j] = integral_pt()->knot(ipt, j);


      // Get position vector to and non-unit tangent vector on wall:
      // dr/ds
      // NOTE: This is before we apply the rigid body motion!
      // so in terms of the write-up the position vector is R_0
      Vector<double> R_0(2);
      Vector<double> drds(2);
      get_non_unit_tangent(s, R_0, drds);

      // Jacobian. Since Jacobian is the same for R, still use it here.
      double J = sqrt(drds[0] * drds[0] + drds[1] * drds[1]);

      // Premultiply the weights and the Jacobian
      double W = w * J;

      // Compute the slender body traction on actual beam; note this is
      // inefficient since we've already computed certain quantities that will
      // be needed in this function
      Vector<double> traction(2);
      compute_slender_body_traction_on_actual_beam(s, traction);

      // Compute R (after translation and rotation)
      Vector<double> R(2);
      R[0] = cos(Theta_eq-theta_initial()) * R_0[0] - sin(Theta_eq-theta_initial()) * R_0[1] + 0.5 * V * t * t +
             U0 * t + X0;
      R[1] = sin(Theta_eq-theta_initial()) * R_0[0] + cos(Theta_eq-theta_initial()) * R_0[1] + V * t + Y0;

      // calculate the contribution to torque
      double local_torque =
        (R[0] - r_centre[0]) * traction[1] - (R[1] - r_centre[1]) * traction[0];

      // Add 'em
      drag[0] += traction[0] * W;
      drag[1] += traction[1] * W;
      torque += local_torque * W;
    }
  }


  /// Overloaded output function
  void output(std::ostream& outfile, const unsigned& n_plot)
  {
    // Local variables
    Vector<double> s(1);

    // Tecplot header info
    outfile << "ZONE I=" << n_plot << std::endl;

    // Set the number of lagrangian coordinates
    unsigned n_lagrangian = Undeformed_beam_pt->nlagrangian();

    // Set the dimension of the global coordinates
    unsigned n_dim = Undeformed_beam_pt->ndim();

    // Find out how many nodes there are
    unsigned n_node = nnode();

    // Find out how many positional dofs there are
    unsigned n_position_dofs = nnodal_position_type();

    Vector<double> R_0(n_dim);

    // # of nodes, # of positional dofs
    Shape psi(n_node, n_position_dofs);

    // Loop over element plot points
    for (unsigned l1 = 0; l1 < n_plot; l1++)
    {
      s[0] = -1.0 + l1 * 2.0 / (n_plot - 1);

      // Get shape functions
      shape(s, psi);

      Vector<double> interpolated_xi(n_lagrangian);
      interpolated_xi[0] = 0.0;

      // Loop over coordinate directions/components of Vector
      for (unsigned i = 0; i < n_dim; i++)
      {
        // Initialise
        R_0[i] = 0.0;
      }

      // Calculate positions
      for (unsigned l = 0; l < n_node; l++)
      {
        // Loop over positional dofs
        for (unsigned k = 0; k < n_position_dofs; k++)
        {
          // Loop over Lagrangian coordinate directions [xi_gen[] are the
          // the *gen*eralised Lagrangian coordinates: node, type, direction]
          for (unsigned i = 0; i < n_lagrangian; i++)
          {
            interpolated_xi[i] +=
              raw_lagrangian_position_gen(l, k, i) * psi(l, k);
          }

          // Loop over components of the deformed position Vector
          for (unsigned i = 0; i < n_dim; i++)
          {
            R_0[i] += raw_dnodal_position_gen_dt(0, l, k, i) * psi(l, k);
          }
        }
      }

      // Get the normal vector N0 at each plotted point
      Vector<double> N_0(n_dim);
      get_normal(s, N_0);

      // Compute slender body traction acting on the actual beam
      Vector<double> traction(n_dim);
      compute_slender_body_traction_on_actual_beam(s, traction);

      // Compute slender body traction acting on the beam in the reference
      // configuration
      Vector<double> traction_0(n_dim);
      compute_slender_body_traction_on_beam_in_reference_configuration(
        s, traction_0);

      // Translate rigid body parameters into meaningful variables
      // (Type=0: first arm, Type=1: second arm.)
      double V = 0.0;
      double U0 = 0.0;
      double Theta_eq = 0.0;
      double X0 = 0.0;
      double Y0 = 0.0;
      if (Type == 0)
      {
        Rigid_body_element_pt->get_parameters(V, U0, Theta_eq, X0, Y0);
      }
      else
      {
        Rigid_body_element_pt->get_parameters_second_arm(
          V, U0, Theta_eq, X0, Y0);
      }

      // Note that we're looking for an pseudo "equilibrium position"
      // where the angle (and the traction!) remain constant while
      // the beam still moves as a rigid body!
      double t = 0.0;

      // Compute R after translation and rotation
      Vector<double> R(n_dim);
      R[0] = cos(Theta_eq-theta_initial()) * R_0[0] - sin(Theta_eq-theta_initial()) * R_0[1] + 0.5 * V * t * t +
             U0 * t + X0;
      R[1] = sin(Theta_eq-theta_initial()) * R_0[0] + cos(Theta_eq-theta_initial()) * R_0[1] + V * t + Y0;

      // Compute normal N after translation and rotation
      Vector<double> N(n_dim);
      N[0] = cos(Theta_eq-theta_initial()) * N_0[0] - sin(Theta_eq-theta_initial()) * N_0[1];
      N[1] = sin(Theta_eq-theta_initial()) * N_0[0] + cos(Theta_eq-theta_initial()) * N_0[1];

      // Output R0 which is clamped at the origin
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << R_0[i] << " ";
      }

      // Output R which is after translation and rotation
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << R[i] << " ";
      }

      // Output unit normal N0
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << N_0[i] << " ";
      }

      // Output unit normal N which is after translation and rotation
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << N[i] << " ";
      }

      // Output traction acting on the beam in the reference configuration
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << traction_0[i] << " ";
      }

      // Output traction acting on the actual beam
      for (unsigned i = 0; i < n_dim; i++)
      {
        outfile << traction[i] << " ";
      }
      outfile << std::endl;
    }
  }

private:
 
  /// Pointer to element that controls the rigid body motion
  RigidBodyElement* Rigid_body_element_pt;

  /// Pointer to non-dimensional coefficient (FSI)
  double* Q_pt;

 /// Pointer to initial rotation of the element when it's in its (otherwise)
 /// undeformed configuration
 double* Theta_initial_pt;
 
};


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


//=============================================================================
/// Compute the beam's centre of mass (defined outside class to avoid
/// forward references)
//=============================================================================
void RigidBodyElement::compute_centre_of_mass(Vector<double>& r_centre,
                                              const unsigned Type)
{
#ifdef PARANOID
  if (r_centre.size() != 2)
  {
    std::ostringstream error_message;
    error_message << "r_centre should have size 2, not " << r_centre.size()
                  << std::endl;

    throw OomphLibError(
      error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
  }
#endif

  // Find number of elements in the mesh (Type=0: first arm, Type=1: second
  // arm.)
  unsigned n_element = 0;
  if (Type == 0)
  {
    n_element = Beam_mesh_pt->nelement();
  }
  else
  {
    n_element = Beam_mesh_second_arm_pt->nelement();
  }

  Vector<double> int_r(2);
  double length = 0.0;
  Vector<double> total_int_r(2);
  double total_length = 0.0;

  // Loop over the elements to compute the sum of elements' contribution to
  // the (\int r ds) and the length of beam (Type=0: first arm, Type=1: second
  // arm.)
  HaoHermiteBeamElement* elem_pt;
  for (unsigned e = 0; e < n_element; e++)
  {
    // Upcast to the specific element type
    if (Type == 0)
    {
      elem_pt =
        dynamic_cast<HaoHermiteBeamElement*>(Beam_mesh_pt->element_pt(e));
      elem_pt->select_first_arm();
    }
    else
    {
      elem_pt = dynamic_cast<HaoHermiteBeamElement*>(
        Beam_mesh_second_arm_pt->element_pt(e));
      elem_pt->select_second_arm();
    }

    // Compute contribution to the the (\int r ds) and length of beam within
    // the e-th element
    elem_pt->compute_contribution_to_int_r_and_length(int_r, length);

    // Sum the elements' contribution to the (\int r ds) and length of beam
    total_int_r[0] += int_r[0];
    total_int_r[1] += int_r[1];
    total_length += length;
  } // end of loop over elements

  // assemble the (\int r ds) and beam length to get the centre of mass
  r_centre[0] = (1.0 / total_length) * total_int_r[0];
  r_centre[1] = (1.0 / total_length) * total_int_r[1];
}


//=============================================================================
/// Compute the drag and torque on the entire beam structure according to
/// slender body theory (Type=0: first arm, Type=1: second arm.)
//=============================================================================
void RigidBodyElement::compute_drag_and_torque(Vector<double>& total_drag,
                                               double& total_torque,
                                               const unsigned Type)
{
#ifdef PARANOID
  if (total_drag.size() != 2)
  {
    std::ostringstream error_message;
    error_message << "total_drag should have size 2, not " << total_drag.size()
                  << std::endl;

    throw OomphLibError(
      error_message.str(), OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
  }
#endif

  // Find number of elements in the mesh (Type=0: first arm, Type=1: second
  // arm.)
  unsigned n_element = 0;
  if (Type == 0)
  {
    n_element = Beam_mesh_pt->nelement();
  }
  else
  {
    n_element = Beam_mesh_second_arm_pt->nelement();
  }

  Vector<double> drag(2);
  double torque = 0.0;

  // Loop over the elements to compute the sum of elements' contribution to
  // the drag and torque on the entire beam structure (Type=0: first arm,
  // Type=1: second arm.)
  HaoHermiteBeamElement* elem_pt;
  for (unsigned e = 0; e < n_element; e++)
  {
    // Upcast to the specific element type
    if (Type == 0)
    {
      elem_pt =
        dynamic_cast<HaoHermiteBeamElement*>(Beam_mesh_pt->element_pt(e));
      elem_pt->select_first_arm();
    }
    else
    {
      elem_pt = dynamic_cast<HaoHermiteBeamElement*>(
        Beam_mesh_second_arm_pt->element_pt(e));
      elem_pt->select_second_arm();
    }

    // Compute contribution to the drag and torque within the e-th element
    elem_pt->compute_contribution_to_drag_and_torque(drag, torque);

    // Sum the elements' contribution to the drag and torque
    total_drag[0] += drag[0];
    total_drag[1] += drag[1];
    total_torque += torque;
  } // end of loop over elements
}


//======start_of_problem_class==========================================
/// Beam problem object
//======================================================================
class ElasticBeamProblem : public Problem
{

public:

  /// Constructor: The arguments are the number of elements and the parameter to
  /// determine the length of the beam
  ElasticBeamProblem(const unsigned& n_elem, const double& q);

  /// Conduct a parameter study
  void parameter_study();

  /// No actions need to be performed after a solve
  void actions_after_newton_solve() {}

  /// No actions need to be performed before a solve
  void actions_before_newton_solve() {}

private:
  /// Pointer to geometric object that represents the beam's undeformed shape
  GeomObject* Undef_beam_pt;

  /// Pointer to RigidBodyElement that actually contains the rigid body data
  RigidBodyElement* Rigid_body_element_pt;

  /// Pointer to beam mesh (first arm)
  OneDLagrangianMesh<HaoHermiteBeamElement>* Beam_mesh_pt;

  /// Pointer to beam mesh (second arm)
  OneDLagrangianMesh<HaoHermiteBeamElement>* Beam_mesh_second_arm_pt;

  /// Pointer to mesh containing the rigid body element
  Mesh* Rigid_body_element_mesh_pt;

}; // end of problem class


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


//=========================================================================
/// Steady, straight 1D line in 2D space
///  \f[ x = 0.0 \f]
///  \f[ y = \zeta \f]
//=========================================================================
class StraightLineVertical : public GeomObject
{
public:
  /// Constructor derives from GeomObject(1, 2)
  StraightLineVertical() : GeomObject(1, 2) {}

  /// Broken copy constructor
  StraightLineVertical(const StraightLineVertical& dummy) = delete;

  /// Broken assignment operator
  void operator=(const StraightLineVertical&) = delete;

  /// Position Vector at Lagrangian coordinate zeta
  void position(const Vector<double>& zeta, Vector<double>& r) const
  {
    r[0] = 0.0;
    r[1] = zeta[0];
  }


  /// Derivative of position Vector w.r.t. to coordinates:
  /// \f$ \frac{dR_i}{d \zeta_\alpha}\f$ = drdzeta(alpha,i).
  /// Evaluated at current time.
  virtual void dposition(const Vector<double>& zeta,
                         DenseMatrix<double>& drdzeta) const
  {
    // Tangent vector
    drdzeta(0, 0) = 0.0;
    drdzeta(0, 1) = 1.0;
  }


  /// 2nd derivative of position Vector w.r.t. to coordinates:
  /// \f$ \frac{d^2R_i}{d \zeta_\alpha d \zeta_\beta}\f$ =
  /// ddrdzeta(alpha,beta,i). Evaluated at current time.
  virtual void d2position(const Vector<double>& zeta,
                          RankThreeTensor<double>& ddrdzeta) const
  {
    // Derivative of tangent vector
    ddrdzeta(0, 0, 0) = 0.0;
    ddrdzeta(0, 0, 1) = 0.0;
  }


  /// Posn Vector and its  1st & 2nd derivatives
  /// w.r.t. to coordinates:
  /// \f$ \frac{dR_i}{d \zeta_\alpha}\f$ = drdzeta(alpha,i).
  /// \f$ \frac{d^2R_i}{d \zeta_\alpha d \zeta_\beta}\f$ =
  /// ddrdzeta(alpha,beta,i).
  /// Evaluated at current time.
  virtual void d2position(const Vector<double>& zeta,
                          Vector<double>& r,
                          DenseMatrix<double>& drdzeta,
                          RankThreeTensor<double>& ddrdzeta) const
  {
    // Position Vector
    r[0] = 0.0;
    r[1] = zeta[0];

    // Tangent vector
    drdzeta(0, 0) = 0.0;
    drdzeta(0, 1) = 1.0;

    // Derivative of tangent vector
    ddrdzeta(0, 0, 0) = 0.0;
    ddrdzeta(0, 0, 1) = 0.0;
  }
};


//=============start_of_constructor=====================================
/// Constructor for elastic beam problem
//======================================================================
ElasticBeamProblem::ElasticBeamProblem(const unsigned& n_elem, const double& q)
{
  // Drift speed and acceleration of horizontal motion
  double V = 0.0;

  // Speed of horizontal motion
  double U0 = 0.0;

  // Beam's inclination
  double Theta_eq = acos(-1.0) / 6.0;

  // x position of clamped point
  double X0 = 2.0;

  // y position of clamped point
  double Y0 = 2.5;

  // Make the RigidBodyElement that stores the parameters for the rigid body
  // motion
  Rigid_body_element_pt = new RigidBodyElement(V, U0, Theta_eq, X0, Y0);

  // Add the rigid body element to its own mesh
  Rigid_body_element_mesh_pt = new Mesh;
  Rigid_body_element_mesh_pt->add_element_pt(Rigid_body_element_pt);

  // Set the undeformed beam shape for two arms (in the reference orientation
  // before applying the rigid body motion)
  Undef_beam_pt = new StraightLineVertical();

  // Create the (Lagrangian!) mesh, using the StraightLineVertical object
  // Undef_beam_pt to specify the initial (Eulerian) position of the
  // nodes. (first arm)
  double length_1 = fabs(q + 0.5);
  Beam_mesh_pt = new OneDLagrangianMesh<HaoHermiteBeamElement>(
    n_elem, length_1, Undef_beam_pt);

  // Pass the pointer of the mesh to the RigidBodyElement class
  // so it can work out the drag and torque on the entire structure (first arm)
  Rigid_body_element_pt->set_pointer_to_beam_mesh(Beam_mesh_pt);

  // Create the (Lagrangian!) mesh, using the StraightLineVertical object
  // Undef_beam_pt to specify the initial (Eulerian) position of the
  // nodes. (second arm)
  double length_2 = fabs(q - 0.5);
  Beam_mesh_second_arm_pt = new OneDLagrangianMesh<HaoHermiteBeamElement>(
    n_elem, length_2, Undef_beam_pt);

  // Pass the pointer of the mesh to the RigidBodyElement class
  // so it can work out the drag and torque on the entire structure (second arm)
  Rigid_body_element_pt->set_pointer_to_beam_second_arm_mesh(
    Beam_mesh_second_arm_pt);

  // Build the problem's global mesh
  add_sub_mesh(Beam_mesh_pt);
  add_sub_mesh(Beam_mesh_second_arm_pt);
  add_sub_mesh(Rigid_body_element_mesh_pt);
  build_global_mesh();

  // Set the boundary conditions: One end of the beam is clamped in space
  // Pin displacements in both x and y directions, and pin the derivative of
  // position Vector w.r.t. to coordinates in x direction. (first arm)
  Beam_mesh_pt->boundary_node_pt(0, 0)->pin_position(0);
  Beam_mesh_pt->boundary_node_pt(0, 0)->pin_position(1);
  Beam_mesh_pt->boundary_node_pt(0, 0)->pin_position(1, 0);

  // Find number of elements in the mesh (first arm)
  unsigned n_element = Beam_mesh_pt->nelement();

  // Loop over the elements to set physical parameters etc. (first arm)
  for (unsigned e = 0; e < n_element; e++)
  {
    // Upcast to the specific element type
    HaoHermiteBeamElement* elem_pt =
      dynamic_cast<HaoHermiteBeamElement*>(Beam_mesh_pt->element_pt(e));

    // Pass the pointer of RigidBodyElement to the each element
    // so we can work out the rigid body motion
    elem_pt->set_pointer_to_rigid_body_element(Rigid_body_element_pt);


    // hierher kill
    // select first arm (assign Type=0)
    elem_pt->select_first_arm();

    // Set physical parameters for each element:
    elem_pt->h_pt() = &Global_Physical_Variables::H;
    elem_pt->q_pt() = &Global_Physical_Variables::Q;

    // Note: no rotation!

    // Set the undeformed shape for each element
    elem_pt->undeformed_beam_pt() = Undef_beam_pt;
    
  } // end of loop over elements


  // Set the boundary conditions: One end of the beam is clamped in space
  // Pin displacements in both x and y directions, and pin the derivative of
  // position Vector w.r.t. to coordinates in x direction. (second arm)
  Beam_mesh_second_arm_pt->boundary_node_pt(0, 0)->pin_position(0);
  Beam_mesh_second_arm_pt->boundary_node_pt(0, 0)->pin_position(1);
  Beam_mesh_second_arm_pt->boundary_node_pt(0, 0)->pin_position(1, 0);

  // Find number of elements in the mesh (second arm)
  n_element = Beam_mesh_second_arm_pt->nelement();

  // Loop over the elements to set physical parameters etc. (second arm)
  for (unsigned e = 0; e < n_element; e++)
  {
    // Upcast to the specific element type
    HaoHermiteBeamElement* elem_pt = dynamic_cast<HaoHermiteBeamElement*>(
      Beam_mesh_second_arm_pt->element_pt(e));

    // Pass the pointer of RigidBodyElement to the each element
    // so we can work out the rigid body motion
    elem_pt->set_pointer_to_rigid_body_element(Rigid_body_element_pt);


    // kill this
    // select second arm (assign Type=1)
    elem_pt->select_second_arm();

    // Set physical parameters for each element:
    elem_pt->h_pt() = &Global_Physical_Variables::H;
    elem_pt->q_pt() = &Global_Physical_Variables::Q;

    // Rotate by opening angle
    elem_pt->theta_initial_pt(&Global_Physical_Variables::Alpha);

    // Set the undeformed shape for each element
    elem_pt->undeformed_beam_pt() = Undef_beam_pt;
    
  } // end of loop over elements

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
  Problem::Max_newton_iterations = 100;

  // Create label for output
  DocInfo doc_info;

  // Set output directory -- this function checks if the output
  // directory exists and issues a warning if it doesn't.
  doc_info.set_directory("RESLT");

  // Output file stream used for writing results
  ofstream file;
  ofstream file2;

  // String used for the filename
  char filename[100];

  // Loop over parameter increments
  unsigned nstep = 1;
  for (unsigned i = 1; i <= nstep; i++)
  {
    // Increment Non-dimensional coefficeient (FSI)
    Global_Physical_Variables::Q = 1.0e-7 * double(i);

    // Solve the system
    newton_solve();

    // Document the solution (first arm)
    sprintf(filename, "RESLT/beam%i.dat", i);
    file.open(filename);
    Beam_mesh_pt->output(file, 5);
    file.close();

    // Document the solution (second arm)
    sprintf(filename, "RESLT/beam_second_arm%i.dat", i);
    file2.open(filename);
    Beam_mesh_second_arm_pt->output(file2, 5);
    file2.close();
  }

} // end of parameter study

//========start_of_main================================================
/// Driver for beam (string under tension) test problem
//=====================================================================
int main()
{
  // Set the non-dimensional thickness
  Global_Physical_Variables::H = 0.01;

  // Number of elements (choose an even number if you want the control point
  // to be located at the centre of the beam)
  unsigned n_element = 10;

  // The parameter to determine the length of the beam
  // first arm length = |q+0.5|, second arm length = |q-0.5|
  double q = 0.1;

  /// Angle between the two arms of the beam
  Global_Physical_Variables::Alpha = acos(-1.0);

  // Construst the problem
  ElasticBeamProblem problem(n_element, q);

  // Check that we're ready to go:
  cout << "\n\n\nProblem self-test ";
  if (problem.self_test() == 0)
  {
    cout << "passed: Problem can be solved." << std::endl;
  }
  else
  {
    throw OomphLibError(
      "Self test failed", OOMPH_CURRENT_FUNCTION, OOMPH_EXCEPTION_LOCATION);
  }

  // Conduct parameter study
  problem.parameter_study();


} // end of main
