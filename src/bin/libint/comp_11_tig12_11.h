
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <assert.h>
#include <dgvertex.h>
#include <rr.h>
#include <integral.h>
#include <tig12_11_11.h>
#include <algebra.h>
#include <flop.h>
#include <prefactors.h>
#include <context.h>
#include <default_params.h>

#ifndef _libint2_src_bin_libint_cr11tig1211_h_
#define _libint2_src_bin_libint_cr11tig1211_h_

using namespace std;


namespace libint2 {

  /** Compute relation for 2-e integrals of the Ti_G12 operators.
  I<BFSet,K> is the integral set specialization that describes the
  integrals of the Ti_G12 operator.
  */
  template <template <class,int> class I, class BFSet, int K>
  class CR_11_TiG12_11 : public RecurrenceRelation,
                         public EnableSafePtrFromThis< CR_11_TiG12_11<I,BFSet,K> >
    {

  public:
    typedef CR_11_TiG12_11<I,BFSet,K> ThisType;
    typedef I<BFSet,K> TargetType;
    typedef R12kG12_11_11<BFSet,0> ChildType;
    /// The type of expressions in which RecurrenceRelations result.
    typedef RecurrenceRelation::ExprType ExprType;

    /** Use Instance() to obtain an instance of RR. This function is provided to avoid
        issues with getting a SafePtr from constructor (as needed for registry to work).
    */
    static SafePtr<ThisType> Instance(const SafePtr<TargetType>&);
    ~CR_11_TiG12_11();

    /// Implementation of RecurrenceRelation::num_children()
    const unsigned int num_children() const { return nchildren_; };
    /// target() returns pointer to the i-th child
    SafePtr<TargetType> target() const { return target_; };
    /// child(i) returns pointer to the i-th child
    SafePtr<ChildType> child(unsigned int i) const;
    /// Implementation of RecurrenceRelation::rr_target()
    SafePtr<DGVertex> rr_target() const { return static_pointer_cast<DGVertex,TargetType>(target()); }
    /// Implementation of RecurrenceRelation::rr_child()
    SafePtr<DGVertex> rr_child(unsigned int i) const { return dynamic_pointer_cast<DGVertex,ChildType>(child(i)); }
    /// Implementation of RecurrenceRelation::rr_expr()
    SafePtr<ExprType> rr_expr() const { return expr_; }
    /// Implementation of RecurrenceRelation::is_simple()
    bool is_simple() const {
      return TrivialBFSet<BFSet>::result;
    }
    /// Implementation of RecurrenceRelation::invariant_type()
    bool invariant_type() const {
      return true;
    }
    /// Implementation of RecurrenceRelation::label()
    const std::string& label() const { return label_; }

    /// Implementation of RecurrenceRelation::nflops()
    unsigned int nflops() const { return nflops_; }
    /// Implementation of RecurrenceRelation::spfunction_call()
    std::string spfunction_call(const SafePtr<CodeContext>& context,
                                const SafePtr<ImplicitDimensions>& dims) const;
    
    const std::string cpp_function_name() {}
    const std::string cpp_source_name() {}
    const std::string cpp_header_name() {}
    std::ostream& cpp_source(std::ostream&) {}

  private:
    /**
      dir specifies which quantum number is incremented.
      For example, dir can be 0 (x), 1(y), or 2(z) if BFSet is
      a Cartesian Gaussian.
     */
    CR_11_TiG12_11(const SafePtr<TargetType>&);

    /// registers this RR with the stack, if needed
    bool register_with_rrstack() const;
    
    static const unsigned int max_nchildren_ = 18;
    
    SafePtr<TargetType> target_;
    SafePtr<ChildType> children_[max_nchildren_];
    SafePtr<ExprType> expr_;

    unsigned int nchildren_;
    unsigned int nexpr_;
    unsigned int nflops_;
 
    std::string label_;
    std::string generate_label(const SafePtr<TargetType>& target) const;

    void add_expr(const SafePtr<ExprType>&, int minus=1);
  };

  template <template <class,int> class I, class F, int K>
    SafePtr< CR_11_TiG12_11<I,F,K> >
    CR_11_TiG12_11<I,F,K>::Instance(const SafePtr<TargetType>& Tint)
    {
      SafePtr<ThisType> this_ptr(new ThisType(Tint));
      // Do post-construction duties
      if (this_ptr->num_children() != 0) {
        this_ptr->register_with_rrstack();
      }
      return this_ptr;
    }

  template <template <class,int> class I, class F, int K>
    bool
    CR_11_TiG12_11<I,F,K>::register_with_rrstack() const
    {
      // only register RRs with for shell sets
      if (TrivialBFSet<F>::result)
        return false;
      SafePtr<RRStack> rrstack = RRStack::Instance();
      SafePtr<ThisType> this_ptr = const_pointer_cast<ThisType,const ThisType>(EnableSafePtrFromThis<ThisType>::SafePtr_from_this());
      rrstack->find(this_ptr);
      return true;
    }
  
  template <template <class,int> class I, class F, int K>
    CR_11_TiG12_11<I,F,K>::CR_11_TiG12_11(const SafePtr<I<F,K> >& Tint) :
    target_(Tint), nchildren_(0), nexpr_(0), nflops_(0), label_(generate_label(Tint))
    {
      F sh_a(Tint->bra(0,0));
      F sh_b(Tint->ket(0,0));
      F sh_c(Tint->bra(1,0));
      F sh_d(Tint->ket(1,0));

      vector<F> bra;
      vector<F> ket;
      bra.push_back(sh_a);
      bra.push_back(sh_c);
      ket.push_back(sh_b);
      ket.push_back(sh_d);

      // On which particle to act
      int p_a = K;
      int p_c = (p_a == 0) ? 1 : 0;

      // Keeps track of the coefficient in front of (ab|cd)
      SafePtr<ExprType> abcd_pfac;
      for(int braket=0; braket<=1; braket++) {
        FunctionPosition where = (FunctionPosition)braket;

        // Use indirection to choose bra or ket
        vector<F>* bra_ref = &bra;
        vector<F>* ket_ref = &ket;
        if (where == InKet) {
          bra_ref = &ket;
          ket_ref = &bra;
        }

        const unsigned int ndirs = is_simple() ? 3 : 1;
        for(int xyz=0; xyz<ndirs; xyz++) {
          
          bool am1_exists = true;
          bool am2_exists = true;
          try {
            bra_ref->operator[](p_a).dec(xyz);
          }
          catch (InvalidDecrement) {
            am1_exists = false;
            am2_exists = false;
          }
          
          if (am1_exists) {
            try {
              bra_ref->operator[](p_a).dec(xyz);
            }
            catch (InvalidDecrement) {
              am2_exists = false;
              bra_ref->operator[](p_a).inc(xyz);
            }
          }
          
          if (am2_exists) {
            int next_child = nchildren_;
            children_[next_child] = ChildType::Instance(bra[0],ket[0],bra[1],ket[1],0);
            nchildren_ += 1;
            if (is_simple()) {
              const unsigned int ni_a = bra_ref->operator[](p_a).qn(xyz) + 2;
              SafePtr<ExprType> expr0_ptr(new ExprType(ExprType::OperatorTypes::Times,prefactors.N_i[ni_a * (ni_a-1)],rr_child(next_child)));
              if (where == InBra)
                add_expr(expr0_ptr);
              else
                add_expr(expr0_ptr,-1);
            }
            nflops_ += ConvertNumFlops<F>(1);
            bra_ref->operator[](p_a).inc(xyz);
            bra_ref->operator[](p_a).inc(xyz);
          }

          // a+2
          {
            bra_ref->operator[](p_a).inc(xyz);
            bra_ref->operator[](p_a).inc(xyz);
            int next_child = nchildren_;
            children_[next_child] = ChildType::Instance(bra[0],ket[0],bra[1],ket[1],0);
            nchildren_ += 1;
            if (is_simple()) {
              SafePtr<ExprType> expr0_ptr(new ExprType(ExprType::OperatorTypes::Times,prefactors.Cdouble(4.0),prefactors.zeta2[K][where]));
              SafePtr<ExprType> expr1_ptr(new ExprType(ExprType::OperatorTypes::Times,expr0_ptr,rr_child(next_child)));
              if (where == InBra)
                add_expr(expr1_ptr);
              else
                add_expr(expr1_ptr,-1);
            }
            nflops_ += ConvertNumFlops<F>(3);
            bra_ref->operator[](p_a).dec(xyz);
            bra_ref->operator[](p_a).dec(xyz);
          }

          
          // a
          {
            if (is_simple()) {
              /*const unsigned int ni_a = bra_ref->operator[](p_a).qn(xyz);
              unsigned int pfac = 2*(ni_a + 1);
              if (am1_exists)
                pfac += 2*ni_a;
              SafePtr<ExprType> pfac_ptr(new ExprType(ExprType::OperatorTypes::Times,prefactors.N_i[pfac],prefactors.zeta[K][where]));
              if (abcd_pfac) {
                abcd_pfac = pfac_ptr;
                nflops_ += ConvertNumFlops<F>(1);
              }
              else {
                SafePtr<ExprType> sum(new ExprType(ExprType::OperatorTypes::Plus,abcd_pfac,pfac_ptr));
                abcd_pfac = pfac_ptr;
                nflops_ += ConvertNumFlops<F>(2);
                }*/
            }
          }
        }
      }

      // now add a
      {
        int next_child = nchildren_;
        children_[next_child] = ChildType::Instance(bra[0],ket[0],bra[1],ket[1],0);
        nchildren_ += 1;
        if (is_simple()) {
          // prefactor in front of (ab|cd) is (4*l_a+6)*zeta_a - (4*l_b+6)*zeta_b
          unsigned int l_x[2];
          for(int braket=0; braket<=1; braket++) {
            FunctionPosition where = (FunctionPosition)braket;
            l_x[braket] = 0;
            for(int xyz=0; xyz<3; xyz++) {
              if (where == InBra)
                l_x[braket] += bra[p_a].qn(xyz);
              else
                l_x[braket] += ket[p_a].qn(xyz);
            }
          }
          SafePtr<ExprType> pfaca_ptr(new ExprType(ExprType::OperatorTypes::Times,prefactors.Cdouble(4*l_x[InBra]+6),prefactors.zeta[K][InBra]));
          SafePtr<ExprType> pfacb_ptr(new ExprType(ExprType::OperatorTypes::Times,prefactors.Cdouble(4*l_x[InKet]+6),prefactors.zeta[K][InKet]));
          SafePtr<ExprType> pfac_ptr(new ExprType(ExprType::OperatorTypes::Minus,pfaca_ptr,pfacb_ptr));
          SafePtr<ExprType> expr_ptr(new ExprType(ExprType::OperatorTypes::Times,pfac_ptr,rr_child(next_child)));
          add_expr(expr_ptr,-1);
        }
        nflops_ += ConvertNumFlops<F>(4);
      }

      // scale by -0.5
      SafePtr<ExprType> scaled(new ExprType(ExprType::OperatorTypes::Times,prefactors.Cdouble(-0.5),expr_));
      expr_ = scaled;
      nflops_ += ConvertNumFlops<F>(1);
    }
  
  template <template <class,int> class I, class F, int K>
    CR_11_TiG12_11<I,F,K>::~CR_11_TiG12_11()
    {
      if (K < 0 || K >= 2) {
        assert(false);
      }
    };

  template <template <class,int> class I, class F, int K>
    SafePtr< typename CR_11_TiG12_11<I,F,K>::ChildType >
    CR_11_TiG12_11<I,F,K>::child(unsigned int i) const
    {
      assert(i>=0 && i<nchildren_);
      unsigned int nc=0;
      for(int c=0; c<max_nchildren_; c++) {
        if (children_[c] != 0) {
          if (nc == i)
            return children_[c];
          nc++;
        }
      }
    };

  template <template <class,int> class I, class F, int K>
    std::string
    CR_11_TiG12_11<I,F,K>::generate_label(const SafePtr<TargetType>& target) const
    {
      ostringstream os;
      
      os << "RR ( ";
      F sh_a(target->bra(0,0)); os << sh_a.label() << " ";
      F sh_b(target->ket(0,0)); os << sh_b.label() << " | [T_" << K << ", G12] | ";
      F sh_c(target->bra(1,0)); os << sh_c.label() << " ";
      F sh_d(target->ket(1,0)); os << sh_d.label() << " )";
      
      return os.str();
    }
    
   template <template <class,int> class I, class F, int K>
    std::string
    CR_11_TiG12_11<I,F,K>::spfunction_call(
    const SafePtr<CodeContext>& context, const SafePtr<ImplicitDimensions>& dims) const
    {
      ostringstream os;
      os << context->label_to_name(label_to_funcname(context->cparams()->api_prefix() + label()))
         // First argument is the library object
         << "(libint, "
         // Second is the target
         << context->value_to_pointer(rr_target()->symbol());
      // then come children
      const unsigned int nchildren = num_children();
      for(int c=0; c<nchildren; c++) {
        os << ", " << context->value_to_pointer(rr_child(c)->symbol());
      }
      os << ")" << context->end_of_stat() << endl;
      return os.str();
    }
    

  template <template <class,int> class I, class F, int K>
    void
    CR_11_TiG12_11<I,F,K>::add_expr(const SafePtr<ExprType>& expr, int minus)
    {
      if (nexpr_ == 0) {
        if (minus != -1) {
          expr_ = expr;
          nexpr_ = 1;
        }
        else {
          SafePtr<ExprType> negative(new ExprType(ExprType::OperatorTypes::Times,expr,prefactors.Cdouble(-1.0)));
          expr_ = negative;
          nflops_ += ConvertNumFlops<F>(1);
        }
      }
      else {
        if (minus != -1) {
          SafePtr<ExprType> sum(new ExprType(ExprType::OperatorTypes::Plus,expr_,expr));
          expr_ = sum;
          nflops_ += ConvertNumFlops<F>(1);
        }
        else {
          SafePtr<ExprType> sum(new ExprType(ExprType::OperatorTypes::Minus,expr_,expr));
          expr_ = sum;
          nflops_ += ConvertNumFlops<F>(1);
        }
      }
    }
    
  /*
  typedef VRR_11_R12kG12_11<R12kG12_11_11,CGShell,0,InBra> VRR_a_11_TwoPRep_11_sh;
  typedef VRR_11_R12kG12_11<R12kG12_11_11,CGShell,1,InBra> VRR_c_11_TwoPRep_11_sh;
  typedef VRR_11_R12kG12_11<R12kG12_11_11,CGShell,0,InKet> VRR_b_11_TwoPRep_11_sh;
  typedef VRR_11_R12kG12_11<R12kG12_11_11,CGShell,1,InKet> VRR_d_11_TwoPRep_11_sh;
  */

};

#endif
