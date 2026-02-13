import lit

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst, FindTool

config.name = "mua"

config.test_format = lit.formats.ShTest(execute_external=False)

config.suffixes = [".mua"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.mua_binary_test_dir

llvm_config.use_default_substitutions()
llvm_config.add_tool_substitutions(
    [
        ToolSubst(
            "%muac",
            command=FindTool("muac"),
            unresolved="fatal",
        ),
    ]
)

llvm_config.with_environment("PATH", config.llvm_tools_dir, append_path=True)
