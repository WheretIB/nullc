#include "InstructionTreeVm.h"

void RunDomTests(ExpressionContext &ctx, VmModule *module)
{
	static bool completed = false;

	if(completed)
		return;

	completed = true;

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[5]);

		blocks[2]->successors.push_back(blocks[3]);
		blocks[2]->successors.push_back(blocks[4]);

		blocks[3]->successors.push_back(blocks[6]);

		blocks[4]->successors.push_back(blocks[6]);

		blocks[5]->successors.push_back(blocks[7]);
		blocks[5]->successors.push_back(blocks[1]);

		blocks[6]->successors.push_back(blocks[7]);

		blocks[7]->successors.push_back(blocks[8]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[1]->dominanceFrontier.size() == 1);
		assert(blocks[1]->dominanceFrontier[0] == blocks[1]);
		assert(blocks[2]->dominanceFrontier.size() == 1);
		assert(blocks[2]->dominanceFrontier[0] == blocks[7]);
		assert(blocks[3]->dominanceFrontier.size() == 1);
		assert(blocks[3]->dominanceFrontier[0] == blocks[6]);
		assert(blocks[4]->dominanceFrontier.size() == 1);
		assert(blocks[4]->dominanceFrontier[0] == blocks[6]);
		assert(blocks[5]->dominanceFrontier.size() == 2);
		assert(blocks[5]->dominanceFrontier[0] == blocks[1]);
		assert(blocks[5]->dominanceFrontier[1] == blocks[7]);
		assert(blocks[6]->dominanceFrontier.size() == 1);
		assert(blocks[6]->dominanceFrontier[0] == blocks[7]);
		assert(blocks[7]->dominanceFrontier.size() == 0);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("11"), 11),
			new VmBlock(ctx.allocator, NULL, InplaceStr("12"), 12),
			new VmBlock(ctx.allocator, NULL, InplaceStr("13"), 13),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[5]);
		blocks[1]->successors.push_back(blocks[9]);

		blocks[2]->successors.push_back(blocks[3]);

		blocks[3]->successors.push_back(blocks[3]);
		blocks[3]->successors.push_back(blocks[4]);

		blocks[4]->successors.push_back(blocks[13]);

		blocks[5]->successors.push_back(blocks[6]);
		blocks[5]->successors.push_back(blocks[7]);

		blocks[6]->successors.push_back(blocks[4]);
		blocks[6]->successors.push_back(blocks[8]);

		blocks[7]->successors.push_back(blocks[8]);
		blocks[7]->successors.push_back(blocks[12]);

		blocks[8]->successors.push_back(blocks[5]);
		blocks[8]->successors.push_back(blocks[13]);

		blocks[9]->successors.push_back(blocks[10]);
		blocks[9]->successors.push_back(blocks[11]);

		blocks[10]->successors.push_back(blocks[12]);

		blocks[11]->successors.push_back(blocks[12]);

		blocks[12]->successors.push_back(blocks[13]);

		blocks[13]->successors.push_back(blocks[14]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[5]->dominanceFrontier.size() == 4);
		assert(blocks[5]->dominanceFrontier[0] == blocks[4]);
		assert(blocks[5]->dominanceFrontier[1] == blocks[5]);
		assert(blocks[5]->dominanceFrontier[2] == blocks[12]);
		assert(blocks[5]->dominanceFrontier[3] == blocks[13]);

		assert(blocks[1]->dominanceChildren.size() == 6);
		assert(blocks[1]->dominanceChildren[0] == blocks[2]);
		assert(blocks[1]->dominanceChildren[1] == blocks[4]);
		assert(blocks[1]->dominanceChildren[2] == blocks[5]);
		assert(blocks[1]->dominanceChildren[3] == blocks[9]);
		assert(blocks[1]->dominanceChildren[4] == blocks[12]);
		assert(blocks[1]->dominanceChildren[5] == blocks[13]);

		assert(blocks[2]->dominanceChildren.size() == 1);
		assert(blocks[2]->dominanceChildren[0] == blocks[3]);

		assert(blocks[5]->dominanceChildren.size() == 3);
		assert(blocks[5]->dominanceChildren[0] == blocks[6]);
		assert(blocks[5]->dominanceChildren[1] == blocks[7]);
		assert(blocks[5]->dominanceChildren[2] == blocks[8]);

		assert(blocks[9]->dominanceChildren.size() == 2);
		assert(blocks[9]->dominanceChildren[0] == blocks[10]);
		assert(blocks[9]->dominanceChildren[1] == blocks[11]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[5]);

		blocks[2]->successors.push_back(blocks[3]);

		blocks[3]->successors.push_back(blocks[3]);
		blocks[3]->successors.push_back(blocks[4]);

		blocks[4]->successors.push_back(blocks[9]);

		blocks[5]->successors.push_back(blocks[6]);
		blocks[5]->successors.push_back(blocks[7]);

		blocks[6]->successors.push_back(blocks[4]);
		blocks[6]->successors.push_back(blocks[8]);

		blocks[7]->successors.push_back(blocks[8]);

		blocks[8]->successors.push_back(blocks[5]);
		blocks[8]->successors.push_back(blocks[9]);

		blocks[9]->successors.push_back(blocks[10]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[5]->dominanceFrontier.size() == 3);
		assert(blocks[5]->dominanceFrontier[0] == blocks[4]);
		assert(blocks[5]->dominanceFrontier[1] == blocks[5]);
		assert(blocks[5]->dominanceFrontier[2] == blocks[9]);
		assert(blocks[5]->dominanceChildren.size() == 3);
		assert(blocks[5]->dominanceChildren[0] == blocks[6]);
		assert(blocks[5]->dominanceChildren[1] == blocks[7]);
		assert(blocks[5]->dominanceChildren[2] == blocks[8]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[7]);

		blocks[2]->successors.push_back(blocks[3]);
		blocks[2]->successors.push_back(blocks[4]);

		blocks[3]->successors.push_back(blocks[5]);

		blocks[4]->successors.push_back(blocks[5]);

		blocks[5]->successors.push_back(blocks[6]);

		blocks[6]->successors.push_back(blocks[11]);

		blocks[7]->successors.push_back(blocks[8]);
		blocks[7]->successors.push_back(blocks[9]);

		blocks[8]->successors.push_back(blocks[10]);

		blocks[9]->successors.push_back(blocks[10]);

		blocks[10]->successors.push_back(blocks[6]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[8]->dominanceFrontier.size() == 1);
		assert(blocks[8]->dominanceFrontier[0] == blocks[10]);
		assert(blocks[9]->dominanceFrontier.size() == 1);
		assert(blocks[9]->dominanceFrontier[0] == blocks[10]);
		assert(blocks[2]->dominanceFrontier.size() == 1);
		assert(blocks[2]->dominanceFrontier[0] == blocks[6]);
		assert(blocks[10]->dominanceFrontier.size() == 1);
		assert(blocks[10]->dominanceFrontier[0] == blocks[6]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[3]);
		blocks[1]->successors.push_back(blocks[4]);

		blocks[2]->successors.push_back(blocks[5]);

		blocks[3]->successors.push_back(blocks[5]);

		blocks[4]->successors.push_back(blocks[6]);

		blocks[5]->successors.push_back(blocks[6]);

		blocks[6]->successors.push_back(blocks[7]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[3]->dominanceFrontier.size() == 1);
		assert(blocks[3]->dominanceFrontier[0] == blocks[5]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("11"), 11),
			new VmBlock(ctx.allocator, NULL, InplaceStr("12"), 12),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);
		blocks[0]->successors.push_back(blocks[13]);

		blocks[1]->successors.push_back(blocks[2]);

		blocks[2]->successors.push_back(blocks[3]);
		blocks[2]->successors.push_back(blocks[7]);

		blocks[3]->successors.push_back(blocks[4]);
		blocks[3]->successors.push_back(blocks[5]);

		blocks[4]->successors.push_back(blocks[6]);

		blocks[5]->successors.push_back(blocks[6]);

		blocks[6]->successors.push_back(blocks[8]);

		blocks[7]->successors.push_back(blocks[8]);

		blocks[8]->successors.push_back(blocks[9]);

		blocks[9]->successors.push_back(blocks[10]);
		blocks[9]->successors.push_back(blocks[11]);

		blocks[10]->successors.push_back(blocks[11]);

		blocks[11]->successors.push_back(blocks[9]);
		blocks[11]->successors.push_back(blocks[12]);

		blocks[12]->successors.push_back(blocks[2]);
		blocks[12]->successors.push_back(blocks[13]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[1]->dominanceFrontier.size() == 1);
		assert(blocks[1]->dominanceFrontier[0] == blocks[13]);

		assert(blocks[2]->dominanceFrontier.size() == 2);
		assert(blocks[2]->dominanceFrontier[0] == blocks[2]);
		assert(blocks[2]->dominanceFrontier[1] == blocks[13]);

		assert(blocks[3]->dominanceFrontier.size() == 1);
		assert(blocks[3]->dominanceFrontier[0] == blocks[8]);

		assert(blocks[4]->dominanceFrontier.size() == 1);
		assert(blocks[4]->dominanceFrontier[0] == blocks[6]);

		assert(blocks[5]->dominanceFrontier.size() == 1);
		assert(blocks[5]->dominanceFrontier[0] == blocks[6]);

		assert(blocks[6]->dominanceFrontier.size() == 1);
		assert(blocks[6]->dominanceFrontier[0] == blocks[8]);

		assert(blocks[7]->dominanceFrontier.size() == 1);
		assert(blocks[7]->dominanceFrontier[0] == blocks[8]);

		assert(blocks[8]->dominanceFrontier.size() == 2);
		assert(blocks[8]->dominanceFrontier[0] == blocks[2]);
		assert(blocks[8]->dominanceFrontier[1] == blocks[13]);

		assert(blocks[9]->dominanceFrontier.size() == 3);
		assert(blocks[9]->dominanceFrontier[0] == blocks[2]);
		assert(blocks[9]->dominanceFrontier[1] == blocks[9]);
		assert(blocks[9]->dominanceFrontier[2] == blocks[13]);

		assert(blocks[10]->dominanceFrontier.size() == 1);
		assert(blocks[10]->dominanceFrontier[0] == blocks[11]);

		assert(blocks[11]->dominanceFrontier.size() == 3);
		assert(blocks[11]->dominanceFrontier[0] == blocks[2]);
		assert(blocks[11]->dominanceFrontier[1] == blocks[9]);
		assert(blocks[11]->dominanceFrontier[2] == blocks[13]);

		assert(blocks[12]->dominanceFrontier.size() == 2);
		assert(blocks[12]->dominanceFrontier[0] == blocks[2]);
		assert(blocks[12]->dominanceFrontier[1] == blocks[13]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("11"), 11),
			new VmBlock(ctx.allocator, NULL, InplaceStr("12"), 12),
			new VmBlock(ctx.allocator, NULL, InplaceStr("13"), 13),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);
		blocks[1]->successors.push_back(blocks[3]);
		blocks[1]->successors.push_back(blocks[4]);

		blocks[2]->successors.push_back(blocks[5]);

		blocks[3]->successors.push_back(blocks[2]);
		blocks[3]->successors.push_back(blocks[5]);
		blocks[3]->successors.push_back(blocks[6]);

		blocks[4]->successors.push_back(blocks[7]);
		blocks[4]->successors.push_back(blocks[8]);

		blocks[5]->successors.push_back(blocks[13]);

		blocks[6]->successors.push_back(blocks[9]);

		blocks[7]->successors.push_back(blocks[10]);

		blocks[8]->successors.push_back(blocks[10]);
		blocks[8]->successors.push_back(blocks[11]);

		blocks[9]->successors.push_back(blocks[6]);
		blocks[9]->successors.push_back(blocks[12]);

		blocks[10]->successors.push_back(blocks[12]);

		blocks[11]->successors.push_back(blocks[10]);

		blocks[12]->successors.push_back(blocks[1]);
		blocks[12]->successors.push_back(blocks[10]);

		blocks[13]->successors.push_back(blocks[9]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[1]->dominanceChildren.size() == 8);
		assert(blocks[1]->dominanceChildren[0] == blocks[2]);
		assert(blocks[1]->dominanceChildren[1] == blocks[3]);
		assert(blocks[1]->dominanceChildren[2] == blocks[4]);
		assert(blocks[1]->dominanceChildren[3] == blocks[5]);
		assert(blocks[1]->dominanceChildren[4] == blocks[6]);
		assert(blocks[1]->dominanceChildren[5] == blocks[9]);
		assert(blocks[1]->dominanceChildren[6] == blocks[10]);
		assert(blocks[1]->dominanceChildren[7] == blocks[12]);

		assert(blocks[4]->dominanceChildren.size() == 2);
		assert(blocks[4]->dominanceChildren[0] == blocks[7]);
		assert(blocks[4]->dominanceChildren[1] == blocks[8]);

		assert(blocks[5]->dominanceChildren.size() == 1);
		assert(blocks[5]->dominanceChildren[0] == blocks[13]);

		assert(blocks[8]->dominanceChildren.size() == 1);
		assert(blocks[8]->dominanceChildren[0] == blocks[11]);
	}

	{
		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, NULL, NULL, VmType::Void);

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("11"), 11),
			new VmBlock(ctx.allocator, NULL, InplaceStr("12"), 12),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[1]->successors.push_back(blocks[2]);

		blocks[2]->successors.push_back(blocks[3]);

		blocks[3]->successors.push_back(blocks[4]);

		blocks[4]->successors.push_back(blocks[5]);
		blocks[4]->successors.push_back(blocks[7]);

		blocks[5]->successors.push_back(blocks[6]);

		blocks[6]->successors.push_back(blocks[3]);
		blocks[6]->successors.push_back(blocks[10]);

		blocks[7]->successors.push_back(blocks[8]);

		blocks[8]->successors.push_back(blocks[9]);
		blocks[8]->successors.push_back(blocks[11]);

		blocks[9]->successors.push_back(blocks[2]);

		blocks[10]->successors.push_back(blocks[12]);

		blocks[11]->successors.push_back(blocks[12]);

		blocks[12]->successors.push_back(blocks[13]);

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[1]->dominanceChildren.size() == 1);
		assert(blocks[1]->dominanceChildren[0] == blocks[2]);

		assert(blocks[2]->dominanceChildren.size() == 1);
		assert(blocks[2]->dominanceChildren[0] == blocks[3]);

		assert(blocks[3]->dominanceChildren.size() == 1);
		assert(blocks[3]->dominanceChildren[0] == blocks[4]);

		assert(blocks[4]->dominanceChildren.size() == 3);
		assert(blocks[4]->dominanceChildren[0] == blocks[5]);
		assert(blocks[4]->dominanceChildren[1] == blocks[7]);
		assert(blocks[4]->dominanceChildren[2] == blocks[12]);

		assert(blocks[5]->dominanceChildren.size() == 1);
		assert(blocks[5]->dominanceChildren[0] == blocks[6]);

		assert(blocks[6]->dominanceChildren.size() == 1);
		assert(blocks[6]->dominanceChildren[0] == blocks[10]);

		assert(blocks[7]->dominanceChildren.size() == 1);
		assert(blocks[7]->dominanceChildren[0] == blocks[8]);

		assert(blocks[8]->dominanceChildren.size() == 2);
		assert(blocks[8]->dominanceChildren[0] == blocks[9]);
		assert(blocks[8]->dominanceChildren[1] == blocks[11]);
	}

	{
		FunctionData *function = new FunctionData(ctx.allocator, NULL, ctx.globalScope, false, false, false, ctx.GetFunctionType(NULL, ctx.typeVoid, IntrusiveList<TypeHandle>()), ctx.typeInt, new SynIdentifier(InplaceStr("f")), IntrusiveList<MatchData>(), 1);

		ScopeData *scopeOuter = new ScopeData(ctx.allocator, NULL, 1, SCOPE_EXPLICIT);

		ScopeData *scope = new ScopeData(ctx.allocator, scopeOuter, 1, function);

		VmFunction *f = new VmFunction(ctx.allocator, VmType::Void, NULL, function, scope, VmType::Void);

		module->currentFunction = f;

		VmBlock *blocks[] = {
			new VmBlock(ctx.allocator, NULL, InplaceStr("entry"), 0),
			new VmBlock(ctx.allocator, NULL, InplaceStr("1"), 1),
			new VmBlock(ctx.allocator, NULL, InplaceStr("2"), 2),
			new VmBlock(ctx.allocator, NULL, InplaceStr("3"), 3),
			new VmBlock(ctx.allocator, NULL, InplaceStr("4"), 4),
			new VmBlock(ctx.allocator, NULL, InplaceStr("5"), 5),
			new VmBlock(ctx.allocator, NULL, InplaceStr("6"), 6),
			new VmBlock(ctx.allocator, NULL, InplaceStr("7"), 7),
			new VmBlock(ctx.allocator, NULL, InplaceStr("8"), 8),
			new VmBlock(ctx.allocator, NULL, InplaceStr("9"), 9),
			new VmBlock(ctx.allocator, NULL, InplaceStr("10"), 10),
			new VmBlock(ctx.allocator, NULL, InplaceStr("11"), 11),
			new VmBlock(ctx.allocator, NULL, InplaceStr("exit"), 100)
		};

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
			f->AddBlock(blocks[i]);

		VariableData *variable = new VariableData(ctx.allocator, NULL, scope, 0, ctx.typeInt, new SynIdentifier(InplaceStr("x")), 0, 1);

		scope->allVariables.push_back(variable);

		blocks[0]->successors.push_back(blocks[1]);

		blocks[0]->AddUse(f);

		{
			module->currentBlock = blocks[0];
			CreateJump(module, nullptr, blocks[1]);
			module->currentBlock = NULL;
		}

		blocks[1]->successors.push_back(blocks[2]);

		{
			module->currentBlock = blocks[1];
			VmInstruction *inst = CreateInstruction(module, NULL, VmType::Void, GetStoreInstruction(ctx, ctx.typeInt), CreateConstantPointer(ctx.allocator, NULL, 0, variable, ctx.typeInt, true), CreateConstantInt(ctx.allocator, NULL, 0), CreateConstantInt(ctx.allocator, NULL, 3));
			inst->comment = InplaceStr("x");
			module->currentBlock = NULL;
		}

		{
			module->currentBlock = blocks[1];
			CreateJump(module, nullptr, blocks[2]);
			module->currentBlock = NULL;
		}

		blocks[2]->successors.push_back(blocks[3]);
		blocks[2]->successors.push_back(blocks[4]);

		{
			module->currentBlock = blocks[2];
			CreateJumpZero(module, nullptr, CreateConstantInt(ctx.allocator, NULL, 1), blocks[3], blocks[4]);
			module->currentBlock = NULL;
		}

		blocks[3]->successors.push_back(blocks[5]);
		blocks[3]->successors.push_back(blocks[6]);

		{
			module->currentBlock = blocks[3];
			CreateJumpZero(module, nullptr, CreateConstantInt(ctx.allocator, NULL, 1), blocks[5], blocks[6]);
			module->currentBlock = NULL;
		}

		blocks[4]->successors.push_back(blocks[7]);
		blocks[4]->successors.push_back(blocks[8]);

		{
			module->currentBlock = blocks[4];
			CreateJumpZero(module, nullptr, CreateConstantInt(ctx.allocator, NULL, 1), blocks[7], blocks[8]);
			module->currentBlock = NULL;
		}

		blocks[5]->successors.push_back(blocks[9]);

		{
			module->currentBlock = blocks[5];
			VmInstruction *inst = CreateInstruction(module, NULL, VmType::Void, GetStoreInstruction(ctx, ctx.typeInt), CreateConstantPointer(ctx.allocator, NULL, 0, variable, ctx.typeInt, true), CreateConstantInt(ctx.allocator, NULL, 0), CreateConstantInt(ctx.allocator, NULL, 4));
			inst->comment = InplaceStr("x");
			module->currentBlock = NULL;
		}

		{
			module->currentBlock = blocks[5];
			CreateJump(module, nullptr, blocks[9]);
			module->currentBlock = NULL;
		}

		blocks[6]->successors.push_back(blocks[9]);

		{
			module->currentBlock = blocks[6];
			CreateJump(module, nullptr, blocks[9]);
			module->currentBlock = NULL;
		}

		blocks[7]->successors.push_back(blocks[10]);

		{
			module->currentBlock = blocks[7];
			CreateJump(module, nullptr, blocks[10]);
			module->currentBlock = NULL;
		}

		blocks[8]->successors.push_back(blocks[10]);

		{
			module->currentBlock = blocks[8];
			VmInstruction *inst = CreateInstruction(module, NULL, VmType::Void, GetStoreInstruction(ctx, ctx.typeInt), CreateConstantPointer(ctx.allocator, NULL, 0, variable, ctx.typeInt, true), CreateConstantInt(ctx.allocator, NULL, 0), CreateConstantInt(ctx.allocator, NULL, 5));
			inst->comment = InplaceStr("x");
			module->currentBlock = NULL;
		}

		{
			module->currentBlock = blocks[8];
			CreateJump(module, nullptr, blocks[10]);
			module->currentBlock = NULL;
		}

		blocks[9]->successors.push_back(blocks[11]);

		{
			module->currentBlock = blocks[9];
			CreateJump(module, nullptr, blocks[11]);
			module->currentBlock = NULL;
		}

		blocks[10]->successors.push_back(blocks[11]);

		{
			module->currentBlock = blocks[10];
			CreateJump(module, nullptr, blocks[11]);
			module->currentBlock = NULL;
		}

		blocks[11]->successors.push_back(blocks[12]);

		{
			module->currentBlock = blocks[11];
			CreateJump(module, nullptr, blocks[12]);
			module->currentBlock = NULL;
		}

		{
			module->currentBlock = blocks[12];

			VmValue *load = CreateLoad(ctx, module, NULL, ctx.typeInt, CreateConstantPointer(ctx.allocator, NULL, 0, variable, ctx.typeInt, true), 0);
			load->comment = InplaceStr("x");

			CreateReturn(module, NULL, load);

			module->currentBlock = NULL;
		}

		for(unsigned i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++)
		{
			VmBlock *b = blocks[i];

			for(unsigned k = 0; k < b->successors.size(); k++)
				b->successors[k]->predecessors.push_back(b);
		}

		f->UpdateDominatorTree(module, false);

		//module->functions.push_back(f);

		assert(blocks[1]->dominanceChildren.size() == 1);
		assert(blocks[1]->dominanceChildren[0] == blocks[2]);

		assert(blocks[2]->dominanceChildren.size() == 3);
		assert(blocks[2]->dominanceChildren[0] == blocks[3]);
		assert(blocks[2]->dominanceChildren[1] == blocks[4]);
		assert(blocks[2]->dominanceChildren[2] == blocks[11]);

		assert(blocks[3]->dominanceChildren.size() == 3);
		assert(blocks[3]->dominanceChildren[0] == blocks[5]);
		assert(blocks[3]->dominanceChildren[1] == blocks[6]);
		assert(blocks[3]->dominanceChildren[2] == blocks[9]);

		assert(blocks[4]->dominanceChildren.size() == 3);
		assert(blocks[4]->dominanceChildren[0] == blocks[7]);
		assert(blocks[4]->dominanceChildren[1] == blocks[8]);
		assert(blocks[4]->dominanceChildren[2] == blocks[10]);

		RunMemoryToRegister(ctx, module, f);

		//RunDeadCodeElimiation(ctx, module, f);

		RunDeadAlocaStoreElimination(ctx, module, f);

		RunPrepareSsaExit(ctx, module, f);

		module->currentFunction = NULL;
	}
}
