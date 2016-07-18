/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#include "IConditional.h"

RTTI_IMPL(IConditional, ISerializable);
RTTI_IMPL(NullCondition, IConditional );
RTTI_IMPL(EqualityCondition, IConditional);
RTTI_IMPL(LogicalCondition, IConditional);

REG_SERIALIZABLE(NullCondition);
REG_SERIALIZABLE(EqualityCondition);
REG_SERIALIZABLE(LogicalCondition );


