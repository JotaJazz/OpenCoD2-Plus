#ifndef MAC_TOOLS_MACTOOLS_H_H
#define MAC_TOOLS_MACTOOLS_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct HICommand;

struct HICommand {
    UInt32 attributes;
    UInt32 commandID;
    struct {
        MenuRef menuRef;
        MenuItemIndex menuItemIndex;
    } menu;
};

#ifdef __cplusplus
}
#endif
#endif
