import asset from '@ohos.security.asset';
import { postQueryAssetPromise, postQueryAssetPromiseResult } from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function postQueryAssetPromiseTest() {
  describe('postQueryFuncPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(async () => {
    })
    afterAll(async () => {
    })
    it('postQueryNotExistData', 0, async () => {
      expect(await postQueryAssetPromise(new Uint8Array(32))).assertTrue()
    })

    it('postQueryInvalidChallengeLen', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(31))
      expect(await postQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      attr.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(33))
      expect(await postQueryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('postQueryBytesUnmatchedType', 0, async () => {
      let tags = [ asset.Tag.AUTH_CHALLENGE ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], 0)
        expect(await postQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await postQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('postQueryUnsupportedTag', 0, async () => {
      let tags = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4,
        asset.Tag.SECRET, asset.Tag.ALIAS, asset.Tag.AUTH_TOKEN
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await postQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let numberTags = [
        asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_TYPE, asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_OFFSET, asset.Tag.SYNC_TYPE,
        asset.Tag.CONFLICT_RESOLUTION, asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.AUTH_VALIDITY_PERIOD,
      ]
      for (let i = 0; i < numberTags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(numberTags[i], 0)
        expect(await postQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let boolTags = [asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT]
      for (let i = 0; i < boolTags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(boolTags[i], false)
        expect(await postQueryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}