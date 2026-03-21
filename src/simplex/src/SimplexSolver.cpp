#include "limo/simplex/SimplexSolver.hpp"

#include <optional>

#include "limo/io/SimplexTableau.hpp"
#include "limo/numerics/Matrix.hpp"

namespace limo::simplex {

using Fraction = limo::numerics::fraction::Fraction;
using Matrix   = limo::numerics::Matrix<Fraction>;
using LP       = limo::core::LinearProgram;

// Builds an (m+1) x (n+1) tableau.  Rows 0..m-1 are constraint rows [A|b],
// row m is the objective row [c_bar | -z] where z = -tableau(m,n).
// Performs full Gauss-Jordan canonicalisation for the given basis so the
// function is correct for both phase-1 (identity initial basis) and phase-2
// (arbitrary basis carried over from phase-1).
static Matrix buildTableau(
    const LP& lp,
    const std::vector<std::size_t>& basis,
    bool negate)
{
    const auto m = lp.rows();
    const auto n = lp.cols();

    Matrix tableau(m + 1, n + 1, Fraction{0});

    for (std::size_t r = 0; r < m; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            tableau(r, c) = lp.constraintMatrix()(r, c);
        }
        tableau(r, n) = lp.rightHandSide()[r];
    }

    for (std::size_t c = 0; c < n; ++c) {
        const Fraction& coeff = lp.objectiveCoefficients()[c];
        tableau(m, c) = negate ? (Fraction{0} - coeff) : coeff;
    }

    for (std::size_t r = 0; r < m; ++r) {
        const std::size_t bc = basis[r];

        const Fraction pivotElem = tableau(r, bc);
        if (pivotElem != Fraction{1}) {
            tableau.scale_row(r, Fraction{1} / pivotElem);
        }

        for (std::size_t r2 = 0; r2 <= m; ++r2) {
            if (r2 == r) continue;
            const Fraction factor = tableau(r2, bc);
            if (factor != Fraction{0}) {
                tableau.add_scaled_row(r2, r, Fraction{0} - factor);
            }
        }
    }

    return tableau;
}

static limo::io::SimplexTableau makeSnapshot(
    std::size_t iterNum,
    int phase,
    const Matrix& tableau,
    const std::vector<std::size_t>& basis,
    std::optional<std::size_t> pivotRow,
    std::optional<std::size_t> pivotCol,
    const Fraction& objValue,
    const std::vector<std::optional<Fraction>>& ratios)
{
    const auto m = basis.size();
    const auto n = tableau.cols() - 1;

    // Include all rows: m constraint rows + 1 objective row
    Matrix fullTableau(m + 1, n + 1, Fraction{0});
    for (std::size_t r = 0; r <= m; ++r) {
        for (std::size_t c = 0; c <= n; ++c) {
            fullTableau(r, c) = tableau(r, c);
        }
    }

    return limo::io::SimplexTableau{
        iterNum,
        phase,
        std::move(fullTableau),
        basis,
        pivotRow,
        pivotCol,
        objValue,
        ratios
    };
}

SimplexSolver::SolveResult SimplexSolver::solve(
    const LP& lp,
    std::vector<std::size_t> initialBasis,
    limo::io::SolverObserver* observer,
    int phaseNumber)
{
    const auto m = lp.rows();
    const auto n = lp.cols();

    const bool negate = (lp.objectiveSense() == LP::ObjectiveSense::Maximize);

    Matrix tableau = buildTableau(lp, initialBasis, negate);
    std::vector<std::size_t> basis = initialBasis;
    std::size_t iterNum = 0;

    while (true) {
        std::optional<std::size_t> enteringCol;
        Fraction minCost{0};

        for (std::size_t c = 0; c < n; ++c) {
            if (tableau(m, c) < minCost) {
                minCost = tableau(m, c);
                enteringCol = c;
            }
        }

        if (!enteringCol) {
            break;
        }

        std::optional<std::size_t> leavingRow;
        Fraction minRatio{0};
        std::vector<std::optional<Fraction>> ratios(m, std::nullopt);

        for (std::size_t r = 0; r < m; ++r) {
            const Fraction aij = tableau(r, *enteringCol);
            if (aij > Fraction{0}) {
                Fraction ratio = tableau(r, n) / aij;
                ratios[r] = ratio;
                if (!leavingRow || ratio < minRatio) {
                    minRatio = ratio;
                    leavingRow = r;
                }
            }
        }

        if (!leavingRow) {
            if (observer) observer->onSolveComplete();
            return {SolveStatus::Unbounded, {}, Fraction{0}, basis};
        }

        if (observer) {
            Fraction z = Fraction{0} - tableau(m, n);
            if (negate) z = Fraction{0} - z;
            observer->onIteration(makeSnapshot(
                iterNum, phaseNumber, tableau, basis,
                *leavingRow, *enteringCol,
                z, ratios));
        }

        const std::size_t pr = *leavingRow;
        const std::size_t pc = *enteringCol;

        tableau.scale_row(pr, Fraction{1} / tableau(pr, pc));

        for (std::size_t r = 0; r <= m; ++r) {
            if (r == pr) continue;
            const Fraction factor = tableau(r, pc);
            if (factor != Fraction{0}) {
                tableau.add_scaled_row(r, pr, Fraction{0} - factor);
            }
        }

        basis[pr] = pc;
        ++iterNum;
    }

    std::vector<value_type> values(n, Fraction{0});
    for (std::size_t r = 0; r < m; ++r) {
        values[basis[r]] = tableau(r, n);
    }

    Fraction objVal = Fraction{0} - tableau(m, n);
    if (negate) objVal = Fraction{0} - objVal;

    if (observer) {
        std::vector<std::optional<Fraction>> noRatios(m, std::nullopt);
        observer->onPhaseComplete(phaseNumber, makeSnapshot(
            iterNum, phaseNumber, tableau, basis,
            std::nullopt, std::nullopt,
            objVal, noRatios));
    }

    return {SolveStatus::Optimal, std::move(values), objVal, std::move(basis)};
}

} // namespace limo::simplex
