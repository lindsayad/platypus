#pragma once
#include "equation_system.h"
#include "gridfunctions.h"
#include "inputs.h"
#include <fstream>
#include <iostream>
#include <memory>

namespace platypus
{

/// Base problem class.
class Problem
{
public:
  Problem() = default;
  virtual ~Problem();

  std::shared_ptr<mfem::ParMesh> _pmesh{nullptr};
  platypus::BCMap _bc_map;
  platypus::Coefficients _coefficients;
  platypus::Outputs _outputs;
  platypus::InputParameters _solver_options;

  std::unique_ptr<mfem::ODESolver> _ode_solver{nullptr};
  std::unique_ptr<mfem::BlockVector> _f{nullptr};

  std::shared_ptr<mfem::Solver> _jacobian_preconditioner{nullptr};
  std::shared_ptr<mfem::Solver> _jacobian_solver{nullptr};
  std::shared_ptr<mfem::NewtonSolver> _nonlinear_solver{nullptr};

  platypus::FECollections _fecs;
  platypus::FESpaces _fespaces;
  platypus::GridFunctions _gridfunctions;

  mfem::Device _device;
  MPI_Comm _comm;
  int _myid;
  int _num_procs;

  /// Returns a pointer to the operator. See derived classes.
  [[nodiscard]] virtual mfem::Operator * GetOperator() const = 0;

  /// Virtual method to construct the operator. Call for default problems.
  virtual void ConstructOperator() = 0;
};

/// ProblemBuilder base class.
class ProblemBuilder
{
public:
  /// NB: delete empty constructor to allow only derived classes to be constructed.
  ProblemBuilder() = delete;

  // Virtual destructor required to prevent leaks.
  virtual ~ProblemBuilder() = default;

  void SetMesh(std::shared_ptr<mfem::ParMesh> pmesh);
  void SetCoefficients(platypus::Coefficients & coefficients);
  void SetDevice(const std::string & dev);

  void AddFESpace(std::string fespace_name,
                  std::string fec_name,
                  int vdim = 1,
                  int ordering = mfem::Ordering::byNODES);
  void AddGridFunction(std::string gridfunction_name, std::string fespace_name);

  void AddBoundaryCondition(std::string bc_name, std::shared_ptr<platypus::BoundaryCondition> bc);

  virtual void RegisterFESpaces() = 0;
  virtual void RegisterGridFunctions() = 0;
  virtual void RegisterCoefficients() = 0;

  virtual void SetOperatorGridFunctions() = 0;
  virtual void ConstructNonlinearSolver();
  virtual void ConstructOperator() = 0;
  virtual void ConstructState() = 0;
  virtual void ConstructTimestepper() = 0;

  virtual void InitializeKernels();

  void InitializeOutputs();

  /// @brief Call @a FinalizeProblem to setup a problem.
  /// @param build_operator Skips @a ConstructOperator step if false. Set this to false if the problem
  /// operator has already been constructed earlier to avoid rebuilding it.
  void FinalizeProblem(bool build_operator = true);

  /// Returns a shared pointer to the problem.
  std::shared_ptr<platypus::Problem> ReturnProblem() { return _problem; }

protected:
  /// Protected constructor. Derived classes must call this constructor.
  ProblemBuilder(platypus::Problem * problem)
    : _problem(std::shared_ptr<platypus::Problem>(problem))
  {
  }

  /// Supported Jacobian solver types.
  enum class SolverType
  {
    HYPRE_PCG,
    HYPRE_GMRES,
    HYPRE_FGMRES,
    HYPRE_AMG,
    SUPER_LU
  };

  /// Structure containing default parameters which can be passed to @a ConstructJacobianSolverWithOptions.
  /// These will be used if the user has not supplied their own values.
  struct SolverParams
  {
    double _tolerance;
    double _abs_tolerance;

    unsigned int _max_iteration;

    int _print_level;
    int _k_dim;
  };

  /// Overridden in derived classes.
  [[nodiscard]] virtual platypus::Problem * GetProblem() const = 0;

  /// Helper template getter with safety check.
  template <class TDerivedProblem>
  [[nodiscard]] TDerivedProblem * GetProblem() const
  {
    if (!_problem)
    {
      MFEM_ABORT("platypus::Problem instance is NULL.");
    }

    return static_cast<TDerivedProblem *>(_problem.get());
  }

  /// Coefficient used in some derived classes.
  mfem::ConstantCoefficient _one_coef{1.0};

private:
  std::shared_ptr<platypus::Problem> _problem{nullptr};
};

/// Interface for problem builders that are constructing problems with an equation system.
class EquationSystemProblemBuilderInterface
{
public:
  EquationSystemProblemBuilderInterface() = default;
  virtual ~EquationSystemProblemBuilderInterface() = default;

  template <class T>
  void AddKernel(std::string var_name, std::shared_ptr<MFEMKernel<T>> kernel)
  {
    GetEquationSystem()->AddTrialVariableNameIfMissing(var_name);
    GetEquationSystem()->AddKernel(var_name, std::move(kernel));
  }

protected:
  /// Implemented in derived classes. Returns a pointer to the problem operator's equation system.
  [[nodiscard]] virtual platypus::EquationSystem * GetEquationSystem() const = 0;
};

} // namespace platypus
