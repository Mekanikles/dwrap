#pragma once

#include <cmath>
#include <functional>

#include <FL/Fl.h>
#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Tree.h>
#include <FL/Fl_Scroll.h>
#include <FL/Fl_Tile.h>

#include "Common.h"
#include "DirectoryDiff.h"

namespace GUI
{
	class CustomTreeItem : public Fl_Tree_Item
	{
	public:
		using Fl_Tree_Item::Fl_Tree_Item;
		const DiffEntry* diffEntry;
	};

	class CustomTree : public Fl_Tree
	{
	public:
		using Fl_Tree::Fl_Tree;
		int GetTreeHeight()
		{
			int totalH = 0;
			for (Fl_Tree_Item* i =first(); i; i = next(i))
			{
				// What a mess
				totalH += 19;
			}

			return totalH;
		}
	};

	struct Widgets
	{
		Fl_Double_Window* window;
		Fl_Scroll* scroll;
		CustomTree* tree1;
		CustomTree* tree2;
	};

	void UpdateSizes(Widgets& widgets)
	{
		int w = widgets.scroll->w();
		int h = widgets.scroll->h();

		int tree1H = widgets.tree1->GetTreeHeight();
		widgets.tree1->resize(0, 0, w / 2, (int)fmax(tree1H, h));

		int tree2H = widgets.tree2->GetTreeHeight();
		widgets.tree2->resize(w / 2, 0, w / 2, (int)fmax(tree2H, h));
	}

	Widgets g_Widgets;
	using DiffCallback = std::function<void(const DiffEntry&)>;
	DiffCallback g_diffCallback;

	void HandleDoubleClick()
	{
		CustomTreeItem* item = (CustomTreeItem*)(g_Widgets.tree1->first_selected_item());
		if (item)
		{
			assert(item->diffEntry);
			g_diffCallback(*item->diffEntry);
		}
	}

	class CustomWindow : public Fl_Double_Window
	{
	public:
		using Fl_Double_Window::Fl_Double_Window;
		virtual void resize(int x, int y, int w, int h) override
		{
			Fl_Double_Window::resize(x, y, w, h);
			UpdateSizes(g_Widgets);
		}

	    virtual int handle(int e) override
	    {
	        switch (e) {
	              case FL_PUSH:
	                if (Fl::event_clicks() > 0)
	                	HandleDoubleClick();
	                break;
	        }
	        return(Fl_Double_Window::handle(e));
	   }

	};

	void StripifyTreeBackground()
	{
		Widgets& widgets = g_Widgets;
		int count = 0;
		for (Fl_Tree_Item* i = widgets.tree1->first(); i; i = widgets.tree1->next(i))
		{
			if (!widgets.tree1->is_open(i))
				continue;
			if (count % 2 == 0)
				i->labelbgcolor(FL_LIGHT3);
			else
				i->labelbgcolor(FL_WHITE);
			count++;
		}
		count = 0;
		for (Fl_Tree_Item* i = widgets.tree2->first(); i; i = widgets.tree2->next(i))
		{
			if (!widgets.tree2->is_open(i))
				continue;
			if (count % 2 == 0)
				i->labelbgcolor(FL_LIGHT3);
			else
				i->labelbgcolor(FL_WHITE);
			count++;
		}
	}

	void TreeCallback(Fl_Widget *w, void *userdata)
	{
		Fl_Tree* tree = (Fl_Tree*)w;
		Fl_Tree* other = tree == g_Widgets.tree1 ? g_Widgets.tree2 : g_Widgets.tree1;

		CustomTreeItem* item = (CustomTreeItem*)(tree->callback_item());
		assert(item);
		if (!item)
			return;

		const DiffEntry* entry = item->diffEntry;
		assert(entry);

		// Find the entry from the other tree
		CustomTreeItem* otherItem = nullptr;
		for (Fl_Tree_Item* i = other->first(); i; i = other->next(i))
		{
			CustomTreeItem* o = (CustomTreeItem*)(i);
			assert(o);
			if (o->diffEntry == entry)
				otherItem = o;
		}

		switch (tree->callback_reason()) 
		{
			case FL_TREE_REASON_DESELECTED: 
			{
				other->deselect(otherItem, 0);
				break;
			}			
			case FL_TREE_REASON_SELECTED: 
			{
				other->select(otherItem, 0);
				break;
			}
		    case FL_TREE_REASON_OPENED:
			{
				other->open(otherItem, 0);
				break;
			}
		    case FL_TREE_REASON_CLOSED:
			{
				other->close(otherItem, 0);
				break;
			}
			default: break;
		}

		StripifyTreeBackground();
	}

	void InitWindow(int windowWidth, int windowHeight, const DirectoryDiffState& diffState, DiffCallback diffCallback)
	{
		g_diffCallback = diffCallback;

		Widgets& widgets = g_Widgets;
		memset(&widgets, 0, sizeof(widgets));
		widgets.window = new CustomWindow(windowWidth, windowHeight);

		widgets.scroll = new Fl_Scroll(0, 0, windowWidth, windowHeight);
		widgets.scroll->type(Fl_Scroll::VERTICAL);

		widgets.tree1 = new CustomTree(0,0,10,10);
		widgets.tree1->showroot(false);
		widgets.tree1->callback(TreeCallback);
		widgets.tree1->connectorstyle(FL_TREE_CONNECTOR_DOTTED);
		widgets.tree1->item_labelfont(FL_HELVETICA);  
		widgets.tree1->item_labelsize(14);  	
		widgets.tree1->item_labelfgcolor(FL_BLACK);

		widgets.tree2 = new CustomTree(300,0,10,10);
		widgets.tree2->showroot(false);
		widgets.tree2->callback(TreeCallback);
		widgets.tree2->item_labelfont(FL_HELVETICA);  
		widgets.tree2->item_labelsize(14);  
		widgets.tree2->item_labelfgcolor(FL_BLACK);
		widgets.tree2->connectorstyle(FL_TREE_CONNECTOR_DOTTED);

		for (const DiffEntry& entry : diffState.sortedEntries)
		{
			CustomTreeItem* leftItem = nullptr;
			CustomTreeItem* rightItem = nullptr;			

			leftItem = new CustomTreeItem(widgets.tree1->prefs());
			leftItem->diffEntry = &entry;
			if (entry.leftFile != nullptr)
			{
				leftItem->label(entry.leftFile->name.c_str());		
				widgets.tree1->add(entry.leftFile->relativePath.c_str(), leftItem);
				if (entry.rightFile == nullptr)
					leftItem->labelcolor(FL_RED);
				else if(entry.differs)
					leftItem->labelcolor(FL_DARK_MAGENTA);	
			}
			else
			{
				leftItem->label(entry.rightFile->name.c_str());
				leftItem->labelcolor(FL_LIGHT2);	
				widgets.tree1->add(entry.rightFile->relativePath.c_str(), leftItem);
			}

			rightItem = new CustomTreeItem(widgets.tree2->prefs());
			rightItem->diffEntry = &entry;
			if (entry.rightFile != nullptr)
			{
				rightItem->label(entry.rightFile->name.c_str());
			 	widgets.tree2->add(entry.rightFile->relativePath.c_str(), rightItem);
				if (entry.leftFile == nullptr)
					rightItem->labelcolor(FL_GREEN);
				else if(entry.differs)
					rightItem->labelcolor(FL_DARK_MAGENTA);
			}
			else
			{
				rightItem->label(entry.leftFile->name.c_str());
				rightItem->labelcolor(FL_LIGHT2);
				widgets.tree2->add(entry.leftFile->relativePath.c_str(), rightItem);	
			}
		}
		
		// Set tree striped backgrounds
		StripifyTreeBackground();

		widgets.scroll->end();

		widgets.window->end();
		widgets.window->resizable(widgets.scroll);
	}

	void CleanUp()
	{
		Widgets& widgets = g_Widgets;
		if (widgets.window)
			delete widgets.window;
		
		memset(&widgets, 0, sizeof(Widgets));
	}

	int Run()
	{
		g_Widgets.window->show();
		Fl::run();
		return EX_OK;
	}
}

