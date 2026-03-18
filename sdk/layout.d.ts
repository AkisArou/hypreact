export interface LayoutBaseProps {
  id?: string;
  class?: string;
}

export type LayoutRenderable = LayoutNode | LayoutRenderable[] | null;
export type LayoutChildren = LayoutRenderable | LayoutRenderable[];

export interface LayoutComponentProps {
  children?: LayoutChildren;
}

export interface WorkspaceProps extends LayoutBaseProps, LayoutComponentProps {}

export interface GroupProps extends LayoutBaseProps, LayoutComponentProps {}

export interface SlotProps extends LayoutBaseProps, LayoutComponentProps {
  take?: number;
}

export interface WindowProps extends LayoutBaseProps, LayoutComponentProps {
  match?: string;
}

export interface WorkspaceNode {
  type: "workspace";
  props?: WorkspaceProps;
  children?: LayoutChild[];
}

export interface GroupNode {
  type: "group";
  props?: GroupProps;
  children?: LayoutChild[];
}

export interface SlotNode {
  type: "slot";
  props?: SlotProps;
  children?: LayoutChild[];
}

export interface WindowNode {
  type: "window";
  props?: WindowProps;
  children?: LayoutChild[];
}

export type LayoutNode = WorkspaceNode | GroupNode | SlotNode | WindowNode;
export type LayoutChild = LayoutNode | null;

export interface LayoutWindow {
  id: string;
  address?: string | null;
  class?: string | null;
  title?: string | null;
  initialClass?: string | null;
  initialTitle?: string | null;
  workspace?: string | null;
  monitor?: string | null;
  floating?: boolean;
  fullscreen?: boolean;
  focused?: boolean;
  pinned?: boolean;
  urgent?: boolean;
  xwayland?: boolean;
}

export interface LayoutContext {
  monitor: {
    name: string;
    width: number;
    height: number;
    x?: number;
    y?: number;
    scale?: number;
    focused?: boolean;
  };
  workspace: {
    name: string;
    id?: number;
    special?: boolean;
    visible?: string[];
    windowCount: number;
  };
  windows: LayoutWindow[];
  focusedWindow?: LayoutWindow | null;
  state?: Record<string, unknown>;
}

export type LayoutFn = (ctx: LayoutContext) => LayoutRenderable;
