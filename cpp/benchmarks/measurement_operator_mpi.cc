#include <sstream>
#include <chrono>
#include <benchmark/benchmark.h>
#include <sopt/mpi/session.h>
#include <sopt/mpi/communicator.h>
#include <sopt/imaging_padmm.h>
#include <sopt/wavelets.h>
#include "purify/operators.h"
#include "purify/directories.h"
#include "purify/pfitsio.h"
#include "benchmarks/utilities.h"

using namespace purify;
using namespace purify::notinstalled;


// ----------------- Degrid operator constructor fixture -----------------------//

class DegridOperatorCtorFixtureMPI : public ::benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State& state) {
    m_imsizex = state.range(0);
    m_imsizey = state.range(0);

    // Generating random uv(w) coverage
    bool newMeasurements = m_uv_data.vis.size()!=state.range(1);
    if (newMeasurements) {
      m_world = sopt::mpi::Communicator::World();
      m_uv_data = b_utilities::random_measurements(state.range(1),m_world);
    }

    // Data needed for the creation of the measurement operator
    const t_real FoV = 1;      // deg
    m_cellsize = FoV / m_imsizex * 60. * 60.;
    m_w_term = false;
  }

  void TearDown(const ::benchmark::State& state) {
  }

  sopt::mpi::Communicator m_world;
  t_uint m_imsizex;
  t_uint m_imsizey;
  utilities::vis_params m_uv_data;
  t_real m_cellsize;
  bool m_w_term;
};


// -------------- Constructor benchmarks -------------------------//

BENCHMARK_DEFINE_F(DegridOperatorCtorFixtureMPI, CtorDistr)(benchmark::State &state) {
  // benchmark the creation of the distributed measurement operator
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    auto sky_measurements = measurementoperator::init_degrid_operator_2d<Vector<t_complex>>(
        m_world, m_uv_data, m_imsizey, m_imsizex, m_cellsize, m_cellsize, 2, 0, 0.0001, "kb", state.range(2), state.range(2),
        "measure", m_w_term);
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }

  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizex * m_imsizey) * sizeof(t_complex));
}

BENCHMARK_DEFINE_F(DegridOperatorCtorFixtureMPI, CtorMPI)(benchmark::State &state) {
  // benchmark the creation of the distributed MPI measurement operator
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    auto sky_measurements = measurementoperator::init_degrid_operator_2d_mpi<Vector<t_complex>>(
        m_world, m_uv_data, m_imsizey, m_imsizex, m_cellsize, m_cellsize, 2, 0, 0.0001, "kb", state.range(2), state.range(2),
        "measure", m_w_term);
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }

  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizex * m_imsizey) * sizeof(t_complex));
  }


// ----------------- Degrid operator application fixtures -----------------------//

class DegridOperatorDirectFixtureMPI : public ::benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State& state) {
    // Keep count of the benchmark repetitions
    m_counter++;
    
    // Reading image from file and create temporary image
    bool newImage = b_utilities::updateImage(state.range(0), m_image, m_imsizex, m_imsizey);

    // Generating random uv(w) coverage
    bool newMeasurements = b_utilities::updateMeasurements(state.range(1), m_uv_data, m_world);

    // Create measurement operators
    bool newKernel = m_kernel!=state.range(2);
    if (newImage || newMeasurements || newKernel) {
      const t_real FoV = 1;      // deg
      const t_real cellsize = FoV / m_imsizex * 60. * 60.;
      const bool w_term = false;
      m_kernel = state.range(2);
      m_degridOperatorDistr = measurementoperator::init_degrid_operator_2d<Vector<t_complex>>(
	  m_world, m_uv_data, m_imsizey, m_imsizex, cellsize, cellsize, 2, 0, 0.0001, "kb", m_kernel, m_kernel,
          "measure", w_term);
      m_degridOperatorMPI = measurementoperator::init_degrid_operator_2d_mpi<Vector<t_complex>>(
	  m_world, m_uv_data, m_imsizey, m_imsizex, cellsize, cellsize, 2, 0, 0.0001, "kb", m_kernel, m_kernel,
          "measure", w_term);
    }
  }
  
  void TearDown(const ::benchmark::State& state) {
  }
  
  t_uint m_counter;
  sopt::mpi::Communicator m_world;
  t_uint m_kernel;

  Image<t_complex> m_image;
  t_uint m_imsizex;
  t_uint m_imsizey;

  utilities::vis_params m_uv_data;

  std::shared_ptr<sopt::LinearTransform<Vector<t_complex>> const> m_degridOperatorDistr;
  std::shared_ptr<sopt::LinearTransform<Vector<t_complex>> const> m_degridOperatorMPI;
};

class DegridOperatorAdjointFixtureMPI : public ::benchmark::Fixture
{
public:
  void SetUp(const ::benchmark::State& state) {
    // Keep count of the benchmark repetitions
    m_counter++;

    // Reading image from file and create temporary image
    bool newImage = b_utilities::updateEmptyImage(state.range(0), m_image, m_imsizex, m_imsizey);

    // Generating random uv(w) coverage
    bool newMeasurements = b_utilities::updateMeasurements(state.range(1), m_uv_data, m_world);

    // Create measurement operators
    bool newKernel = m_kernel!=state.range(2);
    if (newImage || newMeasurements || newKernel) {
      const t_real FoV = 1;      // deg
      const t_real cellsize = FoV / m_imsizex * 60. * 60.;
      const bool w_term = false;
      m_kernel = state.range(2);
      m_degridOperatorDistr = measurementoperator::init_degrid_operator_2d<Vector<t_complex>>(
	        m_world, m_uv_data, m_imsizey, m_imsizex, cellsize, cellsize, 2, 0, 0.0001, "kb", m_kernel, m_kernel,
          "measure", w_term);
      m_degridOperatorMPI = measurementoperator::init_degrid_operator_2d_mpi<Vector<t_complex>>(
	        m_world, m_uv_data, m_imsizey, m_imsizex, cellsize, cellsize, 2, 0, 0.0001, "kb", m_kernel, m_kernel,
          "measure", w_term);
	  }
  }

  void TearDown(const ::benchmark::State& state) {
  }

  t_uint m_counter;
  sopt::mpi::Communicator m_world;
  t_uint m_kernel;

  Vector<t_complex> m_image;
  t_uint m_imsizex;
  t_uint m_imsizey;

  utilities::vis_params m_uv_data;

  std::shared_ptr<sopt::LinearTransform<Vector<t_complex>> const> m_degridOperatorDistr;
  std::shared_ptr<sopt::LinearTransform<Vector<t_complex>> const> m_degridOperatorMPI;
};


// ----------------- Application benchmarks -----------------------//

BENCHMARK_DEFINE_F(DegridOperatorDirectFixtureMPI, Distr)(benchmark::State &state) {
  // Benchmark the application of the distributed operator
  if ((m_counter%10)==1) {
    m_uv_data.vis = (*m_degridOperatorDistr) * Image<t_complex>::Map(m_image.data(), m_image.size(), 1);
  }
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    m_uv_data.vis = (*m_degridOperatorDistr) * Image<t_complex>::Map(m_image.data(), m_image.size(), 1);
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }
  
  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizey * m_imsizex) * sizeof(t_complex));
}

BENCHMARK_DEFINE_F(DegridOperatorAdjointFixtureMPI, Distr)(benchmark::State &state) {
  // Benchmark the application of the adjoint distributed operator
  if ((m_counter%10)==1) {
    m_image = m_degridOperatorDistr->adjoint() * m_uv_data.vis;
  }
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    m_image = m_degridOperatorDistr->adjoint() * m_uv_data.vis;
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }
  
  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizey * m_imsizex) * sizeof(t_complex));
}

BENCHMARK_DEFINE_F(DegridOperatorDirectFixtureMPI, MPI)(benchmark::State &state) {
  // Benchmark the application of the distributed MPI operator
  if ((m_counter%10)==1) {
    m_uv_data.vis = (*m_degridOperatorMPI) * Image<t_complex>::Map(m_image.data(), m_image.size(), 1);
  }
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    m_uv_data.vis = (*m_degridOperatorMPI) * Image<t_complex>::Map(m_image.data(), m_image.size(), 1);
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }
  
  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizey * m_imsizex) * sizeof(t_complex));
}

BENCHMARK_DEFINE_F(DegridOperatorAdjointFixtureMPI, MPI)(benchmark::State &state) {
  // Benchmark the application of the adjoint distributed MPI operator
  if ((m_counter%10)==1) {
    m_image = m_degridOperatorMPI->adjoint() * m_uv_data.vis;
  }
  while(state.KeepRunning()) {
    auto start = std::chrono::high_resolution_clock::now();
    m_image = m_degridOperatorMPI->adjoint() * m_uv_data.vis;
    auto end = std::chrono::high_resolution_clock::now();
    state.SetIterationTime(b_utilities::duration(start,end,m_world));
  }
  
  state.SetBytesProcessed(int64_t(state.iterations()) * (state.range(1) + m_imsizey * m_imsizex) * sizeof(t_complex));
}


// -------------- Register benchmarks -------------------------//

BENCHMARK_REGISTER_F(DegridOperatorCtorFixtureMPI, CtorDistr)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(DegridOperatorDirectFixtureMPI, Distr)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(DegridOperatorAdjointFixtureMPI, Distr)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(DegridOperatorCtorFixtureMPI, CtorMPI)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(DegridOperatorDirectFixtureMPI, MPI)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(DegridOperatorAdjointFixtureMPI, MPI)
//->Apply(b_utilities::Arguments)
->Args({1024,1000000,4})->Args({1024,10000000,4})
->UseManualTime()
->Repetitions(10)->ReportAggregatesOnly(true)
->Unit(benchmark::kMillisecond);
