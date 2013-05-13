/****************************************************************************
**
** Copyright (C) 2013 EPFL-LTS2
** Contact: Kirell Benzi (first.last@epfl.ch)
**
** This file is part of Graphilt.
**
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements
** will be met: http://www.gnu.org/licenses/
**
****************************************************************************/

#ifndef GHT_ENGINE_H
#define GHT_ENGINE_H

#include <vector>

#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/operation_sparse.hpp>
#include <boost/numeric/ublas/lu.hpp>

// Must be set if you want to use ViennaCL algorithms on ublas objects
#define VIENNACL_WITH_UBLAS 1

#include <viennacl/ocl/platform.hpp>
#include <viennacl/meta/result_of.hpp>
#include <viennacl/scalar.hpp>
#include <viennacl/vector.hpp>
#include <viennacl/compressed_matrix.hpp>
#include <viennacl/linalg/prod.hpp>

#include "util/log.h"

namespace bn = boost::numeric;

namespace ght {
namespace core {

class Engine
{
public:
    Engine() {}

    template<typename ScalarType>
    static bool runNaiveCPU( const bn::ublas::compressed_matrix<ScalarType>& laplacian,
                             const bn::ublas::vector<ScalarType>& signal,
                 const std::vector<std::vector<ScalarType> >& coeff,
                 std::vector<std::vector<ScalarType> >& result )
    {
        size_t nbScales = coeff.size();
        bn::ublas::vector<ScalarType> temp(signal.size());
        bn::ublas::vector<ScalarType> res(signal.size());
        result.resize(nbScales);
        // For earch scale
        for( size_t i = 0; i < nbScales; ++i ) {
            result[i].resize(temp.size());
            // For each scale coefficients
            size_t scaleCoeffs = coeff.at(i).size();
            for( size_t j = 1; j < scaleCoeffs; ++j ) {
                if( j == 0 ) {
                    temp = bn::ublas::prod(laplacian, signal);
                }
                else { // Recurrence relation
                    temp = bn::ublas::prod(laplacian, temp);
                }
                res += temp * coeff[i][j];
            }
            std::copy(res.begin(), res.end(), result[i].begin());
        }
        return true;
    }

    template<typename MatrixType, typename VectorType, typename ScalarType>
    static bool runNaiveGPU( const MatrixType& laplacian, const VectorType& signal,
                 const std::vector<std::vector<ScalarType> >& coeff,
                 std::vector<std::vector<ScalarType> >& result )
    {
        // Init GPU values
        size_t nbScales = coeff.size();
        size_t signalSize = signal.size();
        viennacl::vector<ScalarType> gSignal(signalSize);
        viennacl::vector<ScalarType> gCombLaplacian(signalSize);
        viennacl::vector<ScalarType> gResult(signalSize);
        viennacl::compressed_matrix<ScalarType> gLaplacian(signalSize, signalSize);
        viennacl::matrix<ScalarType> gCoeff(nbScales, coeff.at(0).size());
        result.resize(nbScales);
        // Copy
        try {
            viennacl::copy(signal, gSignal);
            viennacl::copy(laplacian, gLaplacian);
            viennacl::copy(coeff, gCoeff);
        } catch( std::exception& e ) {
            LOG(logERROR) << "Engine::runNaiveGPU: " << e.what();
            return false;
        }
        // For earch scale
        for( size_t i = 0; i < nbScales; ++i ) {
            // For each scale coefficients
            result[i].resize(signalSize);
            size_t scaleCoeffs = coeff.at(i).size();
            for( size_t j = 1; j < scaleCoeffs; ++j ) {
                if( j == 0 ) {
                    gCombLaplacian = viennacl::linalg::prod(gLaplacian, gSignal);
                }
                else { // Recurrence relation
                    gCombLaplacian = viennacl::linalg::prod(gLaplacian, gCombLaplacian);
                }
                gResult += gCombLaplacian * gCoeff(i,j);
            }

            // Copy back results
            viennacl::copy(gResult, result[i]);
        }
        return true;
    }

    bool checkFitInGPUMem( size_t matrixSize, size_t signalSize )
    {
//        uint64_t inputSize = matrixSize + signalSize;
//        LOG(logDEBUG) << "Input size: " << inputSize / (1024 * 1024) << " MB";
//        LOG(logDEBUG) << "Max allocable: " << viennacl::ocl::current_device().max_allocable_memory() / (1024 * 1024) << " MB";

//        if( inputSize > viennacl::ocl::current_device().max_allocable_memory() ) {
//            return false;
//        }
        return true;
    }

};

} // end namespace core
} // end namespace ght

#endif // GHT_ENGINE_H
