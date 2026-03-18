export default function layout(ctx) {
  return (
    <workspace
      id="root"
      class={ctx.workspace.special ? "special-workspace" : "workspace-root"}
    />
  );
}
