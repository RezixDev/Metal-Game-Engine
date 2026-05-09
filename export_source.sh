#!/bin/bash

OUTPUT_FILE="project_source.md"

echo "# Project Source Code" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Collect all files first
FILES=$(find . \
  -type d \( -name ".git" -o -name ".idea" -o -name "build" -o -name "metal-cpp" -o -name "Assets.xcassets" -o -name "cmake-build-debug" \) -prune \
  -o -type f \( -name "*.h" -o -name "*.m" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.mm" -o -name "*.metal" -o -name "CMakeLists.txt" -o -name "*.sh" \) -print \
| sort | grep -v "\./$OUTPUT_FILE" | grep -v "\./export_source.sh")

# Generate simple tree structure
echo "## File Tree" >> "$OUTPUT_FILE"
echo '```' >> "$OUTPUT_FILE"
echo "$FILES" | awk -F/ '{
  for (i=2; i<=NF; i++) {
    path = "";
    for (j=2; j<=i; j++) { path = path (j==2?"": "/") $j; }
    if (!seen[path]++) {
      indent = "";
      for (k=2; k<i; k++) indent = indent "  ";
      printf "%s- %s\n", indent, $i;
    }
  }
}' >> "$OUTPUT_FILE"
echo '```' >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Now append file contents
for file in $FILES; do
    rel_path="${file#./}"
    
    echo "## \`$rel_path\`" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    
    # Determine code block language
    ext="${rel_path##*.}"
    lang="cpp"
    if [[ "$ext" == "txt" || "$rel_path" == "CMakeLists.txt" ]]; then
        lang="cmake"
    elif [[ "$ext" == "sh" ]]; then
        lang="bash"
    fi
    
    echo "\`\`\`$lang" >> "$OUTPUT_FILE"
    cat "$file" >> "$OUTPUT_FILE"
    # Ensure there's a newline before the closing backticks
    echo -e "\n\`\`\`\n" >> "$OUTPUT_FILE"
done

echo "Generated $OUTPUT_FILE"
