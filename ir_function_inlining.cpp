/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_function_inlining.cpp
 *
 * Replaces calls to functions with the body of the function.
 */

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_function_inlining.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class ir_function_inlining_visitor : public ir_visitor {
public:
   ir_function_inlining_visitor()
   {
      /* empty */
   }

   virtual ~ir_function_inlining_visitor()
   {
      /* empty */
   }

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_if *);
   /*@}*/
};

class variable_remap : public exec_node {
public:
   variable_remap(const ir_variable *old_var, ir_variable *new_var)
      : old_var(old_var), new_var(new_var)
   {
      /* empty */
   }
   const ir_variable *old_var;
   ir_variable *new_var;
};

class ir_function_cloning_visitor : public ir_visitor {
public:
   ir_function_cloning_visitor(ir_variable *retval)
      : retval(retval)
   {
      /* empty */
   }

   virtual ~ir_function_cloning_visitor()
   {
      /* empty */
   }

   void remap_variable(const ir_variable *old_var, ir_variable *new_var) {
      variable_remap *remap = new variable_remap(old_var, new_var);
      this->remap_list.push_tail(remap);
   }

   ir_variable *get_remapped_variable(ir_variable *var) {
      foreach_iter(exec_list_iterator, iter, this->remap_list) {
	 variable_remap *remap = (variable_remap *)iter.get();

	 if (var == remap->old_var)
	    return remap->new_var;
      }

      /* Not a reapped variable, so a global scoped reference, for example. */
      return var;
   }

   /* List of variable_remap for mapping from original function body variables
    * to inlined function body variables.
    */
   exec_list remap_list;

   /* Return value for the inlined function. */
   ir_variable *retval;

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_if *);
   /*@}*/

   ir_instruction *result;
};

void
ir_function_cloning_visitor::visit(ir_variable *ir)
{
   ir_variable *new_var = ir->clone();

   this->result = new_var;

   this->remap_variable(ir, new_var);
}

void
ir_function_cloning_visitor::visit(ir_loop *ir)
{
   /* FINISHME: Implement loop cloning. */
   assert(0);

   (void)ir;
   this->result = NULL;
}

void
ir_function_cloning_visitor::visit(ir_loop_jump *ir)
{
   /* FINISHME: Implement loop cloning. */
   assert(0);

   (void) ir;
   this->result = NULL;
}


void
ir_function_cloning_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
   this->result = NULL;
}


void
ir_function_cloning_visitor::visit(ir_function *ir)
{
   assert(0);
   (void) ir;
   this->result = NULL;
}

void
ir_function_cloning_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   ir_rvalue *op[2] = {NULL, NULL};

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
      op[operand] = this->result->as_rvalue();
      assert(op[operand]);
   }

   this->result = new ir_expression(ir->operation, ir->type, op[0], op[1]);
}


void
ir_function_cloning_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);

   this->result = new ir_swizzle(this->result->as_rvalue(), ir->mask);
}

void
ir_function_cloning_visitor::visit(ir_dereference *ir)
{
   if (ir->mode == ir_dereference::ir_reference_variable) {
      ir_variable *var = this->get_remapped_variable(ir->variable_referenced());
      this->result = new ir_dereference_variable(var);
   } else if (ir->mode == ir_dereference::ir_reference_array) {
      ir->var->accept(this);

      ir_rvalue *var = this->result->as_rvalue();

      ir->selector.array_index->accept(this);

      ir_rvalue *index = this->result->as_rvalue();

      this->result = new ir_dereference_array(var, index);
   } else {
      assert(ir->mode == ir_dereference::ir_reference_record);

      ir->var->accept(this);

      ir_rvalue *var = this->result->as_rvalue();

      this->result = new ir_dereference_record(var, strdup(ir->selector.field));
   }
}

void
ir_function_cloning_visitor::visit(ir_assignment *ir)
{
   ir_rvalue *lhs, *rhs, *condition = NULL;

   ir->lhs->accept(this);
   lhs = this->result->as_rvalue();

   ir->rhs->accept(this);
   rhs = this->result->as_rvalue();

   if (ir->condition) {
      ir->condition->accept(this);
      condition = this->result->as_rvalue();
   }

   this->result = new ir_assignment(lhs, rhs, condition);
}


void
ir_function_cloning_visitor::visit(ir_constant *ir)
{
   this->result = ir->clone();
}


void
ir_function_cloning_visitor::visit(ir_call *ir)
{
   exec_list parameters;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();

      param->accept(this);
      parameters.push_tail(this->result);
   }

   this->result = new ir_call(ir->get_callee(), &parameters);
}


void
ir_function_cloning_visitor::visit(ir_return *ir)
{
   ir_rvalue *rval;

   assert(this->retval);

   rval = ir->get_value();
   rval->accept(this);
   rval = this->result->as_rvalue();
   assert(rval);

   result = new ir_assignment(new ir_dereference_variable(this->retval), rval,
			      NULL);
}


void
ir_function_cloning_visitor::visit(ir_if *ir)
{
   /* FINISHME: Implement if cloning. */
   assert(0);

   (void) ir;
   result = NULL;
}

bool
automatic_inlining_predicate(ir_instruction *ir)
{
   ir_call *call = ir->as_call();

   if (call && can_inline(call))
      return true;

   return false;
}

bool
do_function_inlining(exec_list *instructions)
{
   bool progress = false;

   do_expression_flattening(instructions, automatic_inlining_predicate);

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_assignment *assign = ir->as_assignment();
      ir_call *call;

      if (assign) {
	 call = assign->rhs->as_call();
	 if (!call || !can_inline(call))
	    continue;

	 /* generates the parameter setup, function body, and returns the return
	  * value of the function
	  */
	 ir_rvalue *rhs = call->generate_inline(ir);
	 assert(rhs);

	 assign->rhs = rhs;
	 progress = true;
      } else if ((call = ir->as_call()) && can_inline(call)) {
	 (void)call->generate_inline(ir);
	 ir->remove();
	 progress = true;
      } else {
	 ir_function_inlining_visitor v;
	 ir->accept(&v);
      }
   }

   return progress;
}

ir_rvalue *
ir_call::generate_inline(ir_instruction *next_ir)
{
   ir_variable **parameters;
   int num_parameters;
   int i;
   ir_variable *retval = NULL;

   num_parameters = 0;
   foreach_iter(exec_list_iterator, iter_sig, this->callee->parameters)
      num_parameters++;

   parameters = new ir_variable *[num_parameters];

   /* Generate storage for the return value. */
   if (this->callee->return_type) {
      retval = new ir_variable(this->callee->return_type, "__retval");
      next_ir->insert_before(retval);
   }

   ir_function_cloning_visitor v = ir_function_cloning_visitor(retval);

   /* Generate the declarations for the parameters to our inlined code,
    * and set up the mapping of real function body variables to ours.
    */
   i = 0;
   exec_list_iterator sig_param_iter = this->callee->parameters.iterator();
   exec_list_iterator param_iter = this->actual_parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      const ir_variable *const sig_param = (ir_variable *) sig_param_iter.get();
      ir_rvalue *param = (ir_rvalue *) param_iter.get();

      /* Generate a new variable for the parameter. */
      parameters[i] = sig_param->clone();
      next_ir->insert_before(parameters[i]);

      v.remap_variable(sig_param, parameters[i]);

      /* Move the actual param into our param variable if it's an 'in' type. */
      if (parameters[i]->mode == ir_var_in ||
	  parameters[i]->mode == ir_var_inout) {
	 ir_assignment *assign;

	 assign = new ir_assignment(new ir_dereference_variable(parameters[i]),
				    param, NULL);
	 next_ir->insert_before(assign);
      }

      sig_param_iter.next();
      param_iter.next();
   }

   /* Generate the inlined body of the function. */
   foreach_iter(exec_list_iterator, iter, callee->body) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      ir->accept(&v);
      assert(v.result);
      next_ir->insert_before(v.result);
   }

   /* Copy back the value of any 'out' parameters from the function body
    * variables to our own.
    */
   i = 0;
   param_iter = this->actual_parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      ir_instruction *const param = (ir_instruction *) param_iter.get();

      /* Move our param variable into the actual param if it's an 'out' type. */
      if (parameters[i]->mode == ir_var_out ||
	  parameters[i]->mode == ir_var_inout) {
	 ir_assignment *assign;

	 assign = new ir_assignment(param->as_rvalue(),
				    new ir_dereference_variable(parameters[i]),
				    NULL);
	 next_ir->insert_before(assign);
      }

      param_iter.next();
   }

   delete(parameters);

   if (retval)
      return new ir_dereference_variable(retval);
   else
      return NULL;
}

void
ir_function_inlining_visitor::visit(ir_variable *ir)
{
   (void) ir;
}


void
ir_function_inlining_visitor::visit(ir_loop *ir)
{
   do_function_inlining(&ir->body_instructions);
}

void
ir_function_inlining_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_function_inlining_visitor::visit(ir_function_signature *ir)
{
   do_function_inlining(&ir->body);
}


void
ir_function_inlining_visitor::visit(ir_function *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      sig->accept(this);
   }
}

void
ir_function_inlining_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_function_inlining_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_function_inlining_visitor::visit(ir_dereference *ir)
{
   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }
   ir->var->accept(this);
}

void
ir_function_inlining_visitor::visit(ir_assignment *ir)
{
   ir->rhs->accept(this);
}


void
ir_function_inlining_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_function_inlining_visitor::visit(ir_call *ir)
{
   (void) ir;
}


void
ir_function_inlining_visitor::visit(ir_return *ir)
{
   (void) ir;
}


void
ir_function_inlining_visitor::visit(ir_if *ir)
{
   ir->condition->accept(this);

   do_function_inlining(&ir->then_instructions);
   do_function_inlining(&ir->else_instructions);
}
