# Creating Releases

This project uses GitHub Actions to automatically build and publish releases.

## Automatic Release Process

### Creating a New Release

1. **Ensure your code is ready for release**
   - All changes are committed and pushed to the main branch
   - The build compiles successfully locally

2. **Create and push a version tag**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

   Tag naming convention: `v{major}.{minor}.{patch}` (e.g., v1.0.0, v1.1.0, v2.0.0)

3. **GitHub Actions will automatically:**
   - Build both x86 and x64 Release versions
   - Package each binary into a ZIP file
   - Create a GitHub release with the tag name
   - Upload the ZIP files as release assets
   - Generate release notes from commits

4. **Find your release**
   - Go to: https://github.com/YOUR_USERNAME/WinMTRGraph/releases
   - The new release will appear with downloadable binaries

## Manual Build (Without Release)

You can also trigger a build manually without creating a release:

1. Go to: https://github.com/YOUR_USERNAME/WinMTRGraph/actions
2. Click on "Build and Release" workflow
3. Click "Run workflow" button
4. Select the branch and click "Run workflow"

This will build the binaries and upload them as artifacts (available for 90 days), but won't create a GitHub release.

## Release Artifacts

Each release includes:
- `WinMTRGraph-v{version}-x86.zip` - 32-bit Windows executable
- `WinMTRGraph-v{version}-x64.zip` - 64-bit Windows executable

## Troubleshooting

### Build Fails
- Check the Actions tab for build logs
- Ensure all source files are committed
- Verify the project builds locally in Visual Studio

### Release Not Created
- Ensure the tag starts with 'v' (e.g., v1.0.0, not 1.0.0)
- Check that GitHub Actions has permission to create releases in your repository settings

## Requirements

The GitHub Actions workflow requires:
- Windows runner (automatically provided by GitHub)
- MSBuild (automatically installed)
- NuGet (automatically installed)

No additional setup or secrets are required!
